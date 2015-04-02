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

    return ctx;
}

void ss_free(ss_ctx *ctx) {
    free(ctx);
}

void ss_log(ss_ctx *ctx, int level, const char *format, ...) {
    // TODO ログ出力時にロックを獲得する。
    if (level <= ctx->logger.level) {
        va_list args;
        va_start(args, format);
        vdprintf(ctx->logger.fd, format, args);
        va_end(args);
    }
}
