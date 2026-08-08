/**
 * backend_stub.h — Internal header for the stub input backend
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BACKEND_STUB_H
#define BACKEND_STUB_H

#include "playos/playos_input.h"

/**
 * Returns 1 if the primary controller is connected, 0 otherwise.
 * Stub: always returns 0 (no controller).
 */
int backend_stub_controller_connected(void);

/**
 * Fills *state with the current controller snapshot.
 * Stub: zeroes the state and returns -1.
 *
 * @return -1 (no controller available).
 */
int backend_stub_get_controller_state(PlayOSControllerState *state);

#endif /* BACKEND_STUB_H */
