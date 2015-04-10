#include "ss.h"

#include <stdarg.h>
#include <stdio.h>

void ss_log(ss_logger *logger, int level, const char *format, ...) {
    if (level <= logger->level) {
        va_list args;
        pthread_mutex_lock(&(logger->mutex));
        va_start(args, format);
        vdprintf(logger->fd, format, args);
        va_end(args);
        pthread_mutex_unlock(&(logger->mutex));
    }
}
