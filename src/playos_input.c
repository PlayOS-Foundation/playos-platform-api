/**
 * playos_input.c — PlayOS logical controller input implementation
 *
 * Dispatches to the configured backend (evdev for Sprint 3).
 * Strips reserved buttons (SYSTEM, QUICK_MENU) from game-facing snapshots.
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_input.h"

#ifdef PLAYOS_BACKEND_EVDEV
#include "backends/backend_evdev.h"
#else
/* Stub backend — always reports no controller */
#endif

/* Reserved button mask: SYSTEM and QUICK_MENU must never reach games. */
#define PLAYOS_RESERVED_BUTTONS (PLAYOS_BUTTON_SYSTEM | PLAYOS_BUTTON_QUICK_MENU)

int playos_input_controller_connected(void)
{
#ifdef PLAYOS_BACKEND_EVDEV
    return backend_evdev_controller_connected();
#else
    return 0;
#endif
}

int playos_input_get_controller_state(PlayOSControllerState *state)
{
    if (!state) return -1;

#ifdef PLAYOS_BACKEND_EVDEV
    if (backend_evdev_get_controller_state(state) != 0)
        return -1;
#else
    /* Stub: zero-initialize */
    state->buttons = 0;
    for (int i = 0; i < PLAYOS_AXIS_COUNT; i++)
        state->axes[i] = 0.0f;
    state->timestamp_us = 0;
    return -1;
#endif

    /* Strip reserved buttons from game-facing snapshots */
    state->buttons &= ~PLAYOS_RESERVED_BUTTONS;

    return 0;
}
