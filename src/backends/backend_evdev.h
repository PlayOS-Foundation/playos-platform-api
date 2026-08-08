/**
 * backend_evdev.h — Internal header for the evdev input backend
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BACKEND_EVDEV_H
#define BACKEND_EVDEV_H

#include "playos/playos_input.h"

/**
 * Returns 1 if the primary controller is connected, 0 otherwise.
 */
int backend_evdev_controller_connected(void);

/**
 * Fills *state with the current controller snapshot.
 * Reserved buttons are NOT stripped here — that's done by the public API layer.
 *
 * @return 0 on success, -1 if no controller is connected or on error.
 */
int backend_evdev_get_controller_state(PlayOSControllerState *state);

#endif /* BACKEND_EVDEV_H */
