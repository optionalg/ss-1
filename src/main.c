#include "ss.h"

#include <stdio.h>
#include <unistd.h>

void cbk(ss_logger *logger, int socket, void *arg) {
    ss_log(logger, SS_LOG_INFO, "Hello %s\n", "World");
}

int main(void) {
    ss_ctx *ctx = NULL;
    int listen_sd = -1;

    ctx = ss_new(cbk, NULL);
    if (!ctx) {
        fprintf(stderr, "failed to allocate memory.\n");
        goto err;
    }

    listen_sd = ss_listen(ctx, 1234);
    if (listen_sd < 0) {
        fprintf(stderr, "failed to listen port.\n");
        goto err;
    }

    if (!ss_run(ctx, listen_sd)) {
        fprintf(stderr, "failed to start server.\n");
        goto err;
    }

    ss_free(ctx);

    return 0;

err:
    if (ctx) {
        ss_free(ctx);
    }
    if (listen_sd < 0) {
        close(listen_sd);
    }

    return -1;
}
