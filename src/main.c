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

#ifdef USE_UNIX_DOMAIN_SOCKET
    unlink("/tmp/ss.sock");
    listen_sd = ss_listen_uds(&ctx, "/tmp/ss.sock");
#else
    listen_sd = ss_listen_tcp(&ctx, "127.0.0.1", 1234);
#endif
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
