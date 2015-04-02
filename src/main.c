#include <stdio.h>

#include "ss.h"

void cbk(ss_ctx *ctx, int socket, void *arg) {
    ss_log(ctx, SS_LOG_INFO, "Hello %s\n", "World");
}

int main(void) {
    ss_ctx *ctx = ss_new(cbk, NULL);
    ss_run(ctx);
    ss_free(ctx);
    return 0;
}
