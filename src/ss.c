#include "ss.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

ss_ctx *ss_new(void) {
    ss_ctx *ctx = malloc(sizeof(ss_ctx));
    if (!ctx) {
        return NULL;
    }

    ctx->logger.level = SS_DEFAULT_LOG_LEVEL;
    ctx->logger.fd = SS_DEFAULT_LOG_FD;
    pthread_mutex_init(&(ctx->logger.mutex), NULL);

    return ctx;
}

void ss_free(ss_ctx *ctx) {
    free(ctx);
}

void ss_log(ss_ctx *ctx, int level, const char *format, ...) {
    if (level <= ctx->logger.level) {
        va_list args;
        pthread_mutex_lock(&(ctx->logger.mutex));
        va_start(args, format);
        vdprintf(ctx->logger.fd, format, args);
        va_end(args);
        pthread_mutex_unlock(&(ctx->logger.mutex));
    }
}
