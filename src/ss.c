#include "ss.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <ev.h>
#include <pthread.h>

ss_ctx *ss_new(ss_cbk cbk, void *cbk_arg) {
    ss_ctx *ctx = malloc(sizeof(ss_ctx));
    if (!ctx) {
        return NULL;
    }

    ctx->cbk = cbk;
    ctx->cbk_arg = cbk_arg;

    ctx->logger.level = SS_DEFAULT_LOG_LEVEL;
    ctx->logger.fd = SS_DEFAULT_LOG_FD;
    pthread_mutex_init(&(ctx->logger.mutex), NULL);

    return ctx;
}

void ss_free(ss_ctx *ctx) {
    free(ctx);
}

static int update_fopts(int fd, int opt) {
    int opts = fcntl(fd, F_GETFL, 0);
    if (opts == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, opts | opt);
}

static int set_nonblock(int sd) {
    return update_fopts(sd, O_NONBLOCK);
}

typedef struct __listen_watcher {
    struct ev_io watcher;
    ss_ctx *ctx;
} listen_watcher;

typedef struct __client_thread {
    pthread_t thread;
    ss_ctx *ctx;
    int sd;
} client_thread;

static void *client_main(void *arg) {
    client_thread *thread = arg;
    ss_ctx *ctx = thread->ctx;
    ss_logger *logger = &ctx->logger;
    int sd = thread->sd;
    int ret = 0;

    ctx->cbk(logger, sd, ctx->cbk_arg);
    pthread_exit(&ret);
    // TODO join and free
}

static void listen_cb(EV_P_ struct ev_io *ew, int events) {
    listen_watcher *lw = (listen_watcher*)ew;
    ss_ctx *ctx = lw->ctx;
    ss_logger *logger = &ctx->logger;
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    int lsd = ew->fd;
    int csd = -1;
    client_thread *thread = NULL;
    int error = 0;

    csd = accept(lsd, (struct sockaddr*)&sin, &slen);
    if (csd < 0) {
        ss_err(logger, "failed to accept client socket: %s\n", strerror(errno));
        goto err;
    }

    thread = malloc(sizeof(client_thread));
    if (!thread) {
        ss_err(logger, "failed to allocate client thread: %s\n", strerror(errno));
        goto err;
    }
    thread->ctx = ctx;
    thread->sd = csd;

    error = pthread_create((pthread_t*)thread, NULL, client_main, thread);
    if (error != 0) {
        ss_err(logger, "failed to create thread: %s\n", strerror(error));
        goto err;
    }

    return;

err:
    if (csd >= 0) {
        close(csd);
    }

    if (thread) {
        free(thread);
    }
}

static void run(ss_ctx *ctx, int sd) {
    ss_logger *logger = &ctx->logger;
    struct ev_loop *loop = EV_DEFAULT;
    listen_watcher lw;
    ev_io *ew = (ev_io*)&lw;

    if (!loop) {
        ss_err(logger, "failed to allocate event loop\n");
        return;
    }

    lw.ctx = ctx;
    ev_io_init(ew, listen_cb, sd, EV_READ);
    ev_io_start(loop, ew);
    ev_loop(loop, 0);
}

bool ss_run(ss_ctx *ctx, int port) {
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

    run(ctx, sd);

    close(sd);

    return true;

err:
    if (sd >= 0) {
        close(sd);
    }

    return false;
}

void ss_log(ss_logger *logger, int level, const char *format, ...) {
    if (level <= logger->level) {
        va_list args;
        pthread_mutex_lock(&(logger->mutex));
        va_start(args, format);
        vdprintf(logger->fd, format, args);
        va_end(args);
        pthread_mutex_unlock(&(logger->mutex));
    }
}
