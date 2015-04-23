#include "ss.h"
#include "thread.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

static void thread_wait_sd(ss_thread *th) {
    pthread_mutex_lock(&th->mutex);
    while (th->sd < 0) {
        pthread_cond_wait(&th->cond, &th->mutex);
    }
    assert(th->sd >= 0);
    pthread_mutex_unlock(&th->mutex);
}

static void thread_reset_sd(ss_thread *th) {
    pthread_mutex_lock(&th->mutex);
    assert(th->sd >= 0);
    th->sd = -1;
    pthread_mutex_unlock(&th->mutex);
}

static void thread_set_sd(ss_thread *th, int sd) {
    pthread_mutex_lock(&th->mutex);
    assert(th->sd < 0);
    th->sd = sd;
    pthread_cond_signal(&th->cond);
    pthread_mutex_unlock(&th->mutex);
}

static void thread_busy(ss_thread *th, int sd) {
    ss_threads *threads = th->threads;

    pthread_mutex_lock(&threads->mutex);

    // set sd
    thread_set_sd(th, sd);

    // link to busy list
    if (threads->busy) {
        ss_thread *busy = threads->busy;
        assert(busy->prev == NULL);
        th->next = busy;
        busy->prev = th;
        threads->busy = th;
    } else {
        threads->busy = th;
    }

    pthread_mutex_unlock(&threads->mutex);
}

static void thread_free(ss_thread *th) {
    ss_threads *threads = th->threads;

    pthread_mutex_lock(&threads->mutex);

    // reset sd
    thread_reset_sd(th);

    // unlink from busy list
    if (th->prev == NULL) {
        assert(threads->busy == th);
        threads->busy = th->next;
    } else {
        assert(threads->busy != th);
        th->prev->next = th->next;
    }
    if (th->next != NULL) {
        th->next->prev = th->prev;
    }
    th->prev = NULL;
    th->next = NULL;

    // link to free list
    if (threads->free) {
        ss_thread *free = threads->free;
        assert(free->prev == NULL);
        th->next = free;
        free->prev = th;
        threads->free = th;
    } else {
        threads->free = th;
    }

    pthread_mutex_unlock(&threads->mutex);
}

static void *thread_main(void *arg) {
    ss_thread *th = arg;
    while (true) {
        thread_wait_sd(th);
        th->cbk(th->logger, th->sd, th->cbk_arg);
        close(th->sd);
        thread_free(th);
    }
    return NULL;
}

static ss_thread *thread_spawn(ss_ctx *ctx) {
    ss_thread *th = NULL;
    ss_logger *logger = &ctx->logger;
    int error;

    th = malloc(sizeof(ss_thread));
    if (!th) {
        ss_err(logger, "failed to allocate thread: %s\n", strerror(errno));
        goto err;
    }
    pthread_cond_init(&th->cond, NULL);
    pthread_mutex_init(&th->mutex, NULL);
    th->sd = -1;
    th->cbk = ctx->cbk;
    th->cbk_arg = ctx->cbk_arg;
    th->logger = logger;
    th->threads = &ctx->threads;
    th->prev = NULL;
    th->next = NULL;

    error = pthread_create((pthread_t*)th, NULL, thread_main, th);
    if (error != 0) {
        ss_err(logger, "failed to start thread: %s\n", strerror(error));
        goto err;
    }

    return th;

err:
    if (th) {
        free(th);
    }

    return NULL;
}

static ss_thread *thread_alloc(ss_ctx *ctx) {
    ss_logger *logger = &ctx->logger;
    ss_threads *threads = &ctx->threads;
    ss_thread *th = NULL;

    pthread_mutex_lock(&threads->mutex);

    if (threads->free) {
        // unlink from free list
        th = threads->free;
        if (th->next) {
            assert(th->next->prev == th);
            th->next->prev = NULL;
        }
        threads->free = th->next;
        th->next = NULL;
        assert(th->prev == NULL);
    } else {
        th = thread_spawn(ctx);
        ss_debug(logger, "new thread spawned: th = %p\n", th);
    }

    pthread_mutex_unlock(&threads->mutex);

    return th;
}

bool thread_run(ss_ctx *ctx, int sd) {
    ss_logger *logger = &ctx->logger;
    ss_thread *th = thread_alloc(ctx);

    if (th) {
        thread_busy(th, sd);
        return true;
    } else {
        ss_err(logger, "failed to allocate client thread\n");
        return false;
    }
}
