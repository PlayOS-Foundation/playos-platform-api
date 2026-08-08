/**
 * playos_logging.c — Structured logging (stub)
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_logging.h"
#include <stdarg.h>

void playos_log(PlayOSLogLevel level, const char *tag, const char *fmt, ...)
{
    (void)level;
    (void)tag;
    (void)fmt;
}

void playos_log_crash_marker(const char *reason)
{
    (void)reason;
}
