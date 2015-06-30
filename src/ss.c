#include "ss.h"
#include "thread.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ev.h>

bool ss_init(ss_ctx *ctx, ss_cbk cbk, void *cbk_arg) {
    ctx->cbk = cbk;
    ctx->cbk_arg = cbk_arg;

    if (!ss_thread_init(&(ctx->threads))) {
        goto err;
    }

    if (!ss_logger_init(&(ctx->logger))) {
        goto err;
    }

    return true;

err:
    return false;
}

void ss_set_logger_cbk(ss_ctx *ctx, ss_logger_cbk cbk, void *arg) {
    ss_logger_set_cbk(&(ctx->logger), cbk, arg);
}

void ss_set_thread_cache_size(ss_ctx *ctx, int size) {
    ss_thread_set_cache_size(&(ctx->threads), size);
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

    ss_debug(logger, "accept new socket(%d)\n", csd);
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

bool bind_listen_set_nonblock(ss_ctx *ctx, int sd, struct sockaddr *addr, socklen_t addrlen) {
    ss_logger *logger = &ctx->logger;

    if (bind(sd, addr, addrlen) < 0) {
        ss_err(logger, "failed to bind: %s\n", strerror(errno));
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

    return true;

err:
    return false;
}

int ss_listen_tcp(ss_ctx *ctx, const char *ip, int port) {
    int sd = -1;
    struct sockaddr_in sin;
    struct in_addr addr;
    ss_logger *logger = &ctx->logger;

    if (!inet_aton(ip, &addr)) {
        ss_err(logger, "invalid ip: %s\n", ip);
        goto err;
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        ss_err(logger, "failed to create listen socket: %s\n", strerror(errno));
        goto err;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr = addr;
    if (!bind_listen_set_nonblock(ctx, sd, (struct sockaddr*)&sin, sizeof(sin))) {
        goto err;
    }

    return sd;

err:
    if (sd >= 0) {
        close(sd);
    }

    return -1;
}

int ss_listen_uds(ss_ctx *ctx, const char *path) {
    int sd = -1;
    struct sockaddr_un sun;
    ss_logger *logger = &ctx->logger;

    sd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sd < 0) {
        ss_err(logger, "failed to create listen socket: %s\n", strerror(errno));
        goto err;
    }

    if (strlen(path) + 1 > sizeof(sun.sun_path)) {
        ss_err(logger, "too long path: %s\n", path);
        goto err;
    }

    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);
    if (!bind_listen_set_nonblock(ctx, sd, (struct sockaddr*)&sun, sizeof(sun))) {
        goto err;
    }

    return sd;

err:
    if (sd >= 0) {
        close(sd);
    }

    return -1;
}
