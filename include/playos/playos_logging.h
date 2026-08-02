/**
 * playos_logging.h — PlayOS structured logging
 *
 * Log output is written to /data/logs/<game-id>/session-<timestamp>.log
 * and to the kernel ring buffer in development builds.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_LOGGING_H
#define PLAYOS_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLAYOS_LOG_DEBUG = 0,
    PLAYOS_LOG_INFO  = 1,
    PLAYOS_LOG_WARN  = 2,
    PLAYOS_LOG_ERROR = 3
} PlayOSLogLevel;

/**
 * Log a structured message.
 *
 * @param  level  Log severity level.
 * @param  tag    Short category string, e.g. "audio", "render", "save".
 *                Must be a static string literal when possible.
 * @param  fmt    printf-style format string.
 * @param  ...    Format arguments.
 */
void playos_log(PlayOSLogLevel level, const char *tag, const char *fmt, ...);

/**
 * Mark a crash point in the log before calling abort() or raising a signal.
 * Call this before any fatal crash to ensure the reason is persisted.
 *
 * @param  reason  Short human-readable crash description.
 */
void playos_log_crash_marker(const char *reason);

/* Convenience macros */
#define PLAYOS_LOG_D(tag, ...) playos_log(PLAYOS_LOG_DEBUG, (tag), __VA_ARGS__)
#define PLAYOS_LOG_I(tag, ...) playos_log(PLAYOS_LOG_INFO,  (tag), __VA_ARGS__)
#define PLAYOS_LOG_W(tag, ...) playos_log(PLAYOS_LOG_WARN,  (tag), __VA_ARGS__)
#define PLAYOS_LOG_E(tag, ...) playos_log(PLAYOS_LOG_ERROR, (tag), __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* PLAYOS_LOGGING_H */
