#ifndef __SS_SRC_SS_H__
#define __SS_SRC_SS_H__

#include <stdbool.h>
#include <pthread.h>

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
    pthread_mutex_t mutex;
} ss_logger;

typedef void (*ss_cbk)(ss_logger *logger, int socket, void *arg);

struct __ss_thread;
struct __ss_threads;
typedef struct __ss_thread {
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int sd;
    ss_cbk cbk;
    void *cbk_arg;
    ss_logger *logger;
    struct __ss_threads *threads;
    struct __ss_thread *prev;
    struct __ss_thread *next;
} ss_thread;

typedef struct __ss_threads {
    ss_thread *busy;
    ss_thread *free;
    pthread_mutex_t mutex;
} ss_threads;

typedef struct __ss_ctx {
    ss_cbk cbk;
    void *cbk_arg;
    ss_threads threads;
    ss_logger logger;
} ss_ctx;

ss_ctx *ss_new(ss_cbk cbk, void *cbk_arg);
bool ss_init(ss_ctx *ctx, ss_cbk cbk, void *cbk_arg);
int ss_listen_tcp(ss_ctx *ctx, const char *ip, int port);
int ss_listen_uds(ss_ctx *ctx, const char *path);
void ss_free(ss_ctx *ctx);
bool ss_run(ss_ctx *ctx, int listen_fd);
void ss_log(ss_logger *logger, int level, const char *format, ...);

#define ss_err(logger, ...) ss_log((logger), SS_LOG_ERROR, __VA_ARGS__)
#define ss_info(logger, ...) ss_log((logger), SS_LOG_INFO, __VA_ARGS__)
#define ss_debug(logger, ...) ss_log((logger), SS_LOG_DEBUG, __VA_ARGS__)

#endif
