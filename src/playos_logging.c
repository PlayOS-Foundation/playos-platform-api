/**
 * playos_logging.c — Structured logging (real implementation)
 *
 * Log output is written to stderr with [LEVEL] [tag] format.
 * In production, stderr is redirected to /data/log/ by playos-init.
 *
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 199309L
#include "playos/playos_logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static const char *level_string(PlayOSLogLevel level)
{
    switch (level) {
    case PLAYOS_LOG_DEBUG: return "DEBUG";
    case PLAYOS_LOG_INFO:  return "INFO";
    case PLAYOS_LOG_WARN:  return "WARN";
    case PLAYOS_LOG_ERROR: return "ERROR";
    default:               return "????";
    }
}

void playos_log(PlayOSLogLevel level, const char *tag, const char *fmt, ...)
{
    /* Timestamp for each log line */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    fprintf(stderr, "[%6ld.%03ld] [%s] [%s] ",
            (long)ts.tv_sec, (long)(ts.tv_nsec / 1000000),
            level_string(level), tag ? tag : "-");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);
}

void playos_log_crash_marker(const char *reason)
{
    FILE *f = fopen("/data/log/crash_marker", "a");
    if (!f)
        return;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    fprintf(f, "%ld.%03ld CRASH: %s\n",
            (long)ts.tv_sec, (long)(ts.tv_nsec / 1000000),
            reason ? reason : "unknown");
    fclose(f);

    /* Also log to stderr for immediate visibility */
    fprintf(stderr, "[CRASH] %s\n", reason ? reason : "unknown");
    fflush(stderr);
}
