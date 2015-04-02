#include <stdio.h>

#include "ss.h"

int main(void) {
    ss_ctx *ctx = ss_new();
    ss_log(ctx, SS_LOG_INFO, "Hello %s\n", "World");
    ss_free(ctx);
    return 0;
}
