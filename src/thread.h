#ifndef __SS_SRC_THREAD_H__
#define __SS_SRC_THREAD_H__

#include "ss.h"

bool ss_thread_init(ss_threads *threads);
void ss_thread_set_cache_size(ss_threads *threads, int size);
bool thread_run(ss_ctx *ctx, int sd);

#endif
