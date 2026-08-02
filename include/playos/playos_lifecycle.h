/**
 * playos_lifecycle.h — PlayOS lifecycle events
 *
 * Games must poll for lifecycle events each frame and respond
 * to BACKGROUND and TERMINATE promptly.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_LIFECYCLE_H
#define PLAYOS_LIFECYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLAYOS_LIFECYCLE_FOREGROUND,  /**< Game is now the active foreground surface. Resume rendering. */
    PLAYOS_LIFECYCLE_BACKGROUND,  /**< Game is hidden. Pause, mute audio, reduce CPU to near-zero. */
    PLAYOS_LIFECYCLE_SUSPEND,     /**< System suspending. Save state immediately. Return within 500ms. */
    PLAYOS_LIFECYCLE_RESUME,      /**< System resumed from suspend. Restore state. */
    PLAYOS_LIFECYCLE_TERMINATE    /**< Ordered shutdown. Save, release resources, exit(0) within 2s. */
} PlayOSLifecycleEvent;

/**
 * Non-blocking poll for the next lifecycle event.
 *
 * @param[out] event  Filled with the next event if one is pending.
 * @return  1 if an event was written, 0 if no event pending, -1 on error.
 */
int playos_lifecycle_poll(PlayOSLifecycleEvent *event);

/**
 * Blocking wait for the next lifecycle event.
 *
 * @param[out] event       Filled with the next event.
 * @param      timeout_ms  Max wait in milliseconds. -1 = wait indefinitely.
 * @return  1 if an event was written, 0 on timeout, -1 on error.
 */
int playos_lifecycle_wait(PlayOSLifecycleEvent *event, int timeout_ms);

/**
 * Returns the underlying file descriptor for use with poll(2) / select(2).
 * The fd becomes readable when a lifecycle event is available.
 */
int playos_lifecycle_fd(void);

#ifdef __cplusplus
}
#endif

#endif /* PLAYOS_LIFECYCLE_H */
