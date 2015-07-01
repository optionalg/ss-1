#ifndef __SS_SRC_LOGGER_H__
#define __SS_SRC_LOGGER_H__

#include "ss.h"

bool ss_logger_init(ss_logger *logger);
void ss_logger_set_cbk(ss_logger *logger, ss_logger_cbk cbk, void *arg);
void ss_logger_set_log_level(ss_logger *logger, int level);

#endif
