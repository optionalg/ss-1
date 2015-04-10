#ifndef __SS_THREAD_H__
#define __SS_THREAD_H__

#include "ss.h"

void thread_busy(ss_thread *th, int sd);
ss_thread *thread_alloc(ss_ctx *ctx);

#endif
