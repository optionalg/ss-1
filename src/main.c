#include "ss.h"

#include <stdio.h>
#include <unistd.h>

void cbk(ss_logger *logger, int socket, void *arg) {
    ss_log(logger, SS_LOG_INFO, "Hello %s\n", "World");
}

int main(void) {
    ss_ctx ctx;
    int listen_sd = -1;

    if (!ss_init(&ctx, cbk, NULL)) {
        fprintf(stderr, "failed to initialize.\n");
        goto err;
    }

    listen_sd = ss_listen(&ctx, 1234);
    if (listen_sd < 0) {
        fprintf(stderr, "failed to listen port.\n");
        goto err;
    }

    if (!ss_run(&ctx, listen_sd)) {
        fprintf(stderr, "failed to start server.\n");
        goto err;
    }

    close(listen_sd);

    return 0;

err:
    if (listen_sd < 0) {
        close(listen_sd);
    }

    return -1;
}
