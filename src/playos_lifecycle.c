/**
 * playos_lifecycle.c — Lifecycle events (stub)
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_lifecycle.h"

int playos_lifecycle_poll(PlayOSLifecycleEvent *event)
{
    (void)event;
    return 0; /* no event pending */
}

int playos_lifecycle_wait(PlayOSLifecycleEvent *event, int timeout_ms)
{
    (void)event;
    (void)timeout_ms;
    return 0; /* timeout */
}

int playos_lifecycle_fd(void)
{
    return -1;
}
