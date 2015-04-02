#ifndef __SS_H__
#define __SS_H__

#define SS_LOG_FATAL 0
#define SS_LOG_ERROR 1
#define SS_LOG_WARN  2
#define SS_LOG_INFO  3
#define SS_LOG_DEBUG 4
#define SS_LOG_TRACE 5

#define SS_DEFAULT_LOG_LEVEL SS_LOG_INFO
#define SS_DEFAULT_LOG_FD 2 // stderr

typedef struct __ss_logger {
    int level;
    int fd;
} ss_logger;

typedef struct __ss_ctx {
    ss_logger logger;
} ss_ctx;

ss_ctx *ss_new(void);
void ss_free(ss_ctx *ctx);
void ss_log(ss_ctx *ctx, int level, const char *format, ...);

#endif
