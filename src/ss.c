#include "ss.h"
#include "thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <ev.h>

bool ss_init(ss_ctx *ctx, ss_cbk cbk, void *cbk_arg) {
    ctx->cbk = cbk;
    ctx->cbk_arg = cbk_arg;

    ctx->threads.busy = NULL;
    ctx->threads.free = NULL;
    if (pthread_mutex_init(&(ctx->threads.mutex), NULL) != 0) {
        goto err;
    }

    ctx->logger.level = SS_DEFAULT_LOG_LEVEL;
    ctx->logger.fd = SS_DEFAULT_LOG_FD;
    if (pthread_mutex_init(&(ctx->logger.mutex), NULL) != 0) {
        goto err;
    }

    return true;

err:
    return false;
}

ss_ctx *ss_new(ss_cbk cbk, void *cbk_arg) {
    ss_ctx *ctx = malloc(sizeof(ss_ctx));

    if (!ctx) {
        goto err;
    }
    if (!ss_init(ctx, cbk, cbk_arg)) {
        goto err;
    }

    return ctx;

err:
    if (ctx) {
        free(ctx);
    }

    return NULL;
}

void ss_free(ss_ctx *ctx) {
    free(ctx);
}

static int set_fopts(int fd, int opt) {
    int opts = fcntl(fd, F_GETFL, 0);
    if (opts == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, opts | opt);
}

static int clear_fopts(int fd, int opt) {
    int opts = fcntl(fd, F_GETFL, 0);
    if (opts == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, opts & ~opt);
}

static int set_nonblock(int sd) {
    return set_fopts(sd, O_NONBLOCK);
}

static int set_block(int sd) {
    return clear_fopts(sd, O_NONBLOCK);
}

typedef struct __listen_watcher {
    struct ev_io watcher;
    ss_ctx *ctx;
} listen_watcher;

static void listen_cb(EV_P_ struct ev_io *ew, int events) {
    listen_watcher *lw = (listen_watcher*)ew;
    ss_ctx *ctx = lw->ctx;
    ss_logger *logger = &ctx->logger;
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    int lsd = ew->fd;
    int csd = -1;
    ss_thread *th = NULL;

    csd = accept(lsd, (struct sockaddr*)&sin, &slen);
    if (csd < 0) {
        ss_err(logger, "failed to accept client socket: %s\n", strerror(errno));
        goto err;
    }

    if (set_block(csd) < 0) {
        ss_err(logger, "failed to set block: %s\n", strerror(errno));
        goto err;
    }

    ss_info(logger, "accept new socket(%d)\n", csd);
    if (!thread_run(ctx, csd)) {
        ss_err(logger, "failed to run client thread\n");
        goto err;
    }

    return;

err:
    if (csd >= 0) {
        close(csd);
    }
}

bool ss_run(ss_ctx *ctx, int listen_sd) {
    ss_logger *logger = &ctx->logger;
    struct ev_loop *loop = EV_DEFAULT;
    listen_watcher lw;
    ev_io *ew = (ev_io*)&lw;

    if (!loop) {
        ss_err(logger, "failed to allocate event loop\n");
        return false;
    }

    lw.ctx = ctx;
    ev_io_init(ew, listen_cb, listen_sd, EV_READ);
    ev_io_start(loop, ew);
    ev_loop(loop, 0);

    return true;
}

int ss_listen(ss_ctx *ctx, int port) {
    int sd = -1;
    struct sockaddr_in sin;
    ss_logger *logger = &ctx->logger;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        ss_err(logger, "failed to create listen socket: %s\n", strerror(errno));
        goto err;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    // TODO listenするアドレスを選べるようにする。
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        ss_err(logger, "failed to bind, port = %d: %s\n", port, strerror(errno));
        goto err;
    }

    // TODO バックログの値を設定可能にする。
    if (listen(sd, SOMAXCONN) < 0) {
        ss_err(logger, "failed to listen: %s\n", strerror(errno));
        goto err;
    }

    if (set_nonblock(sd) < 0) {
        ss_err(logger, "failed to set nonblock: %s\n", strerror(errno));
        goto err;
    }

    return sd;

err:
    if (sd >= 0) {
        close(sd);
    }

    return -1;
}
