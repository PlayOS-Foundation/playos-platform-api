/**
 * backend_stub.c — Stub input backend (always reports no controller)
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend_stub.h"
#include <string.h>

int backend_stub_controller_connected(void)
{
    return 0;
}

int backend_stub_get_controller_state(PlayOSControllerState *state)
{
    if (!state) return -1;
    memset(state, 0, sizeof(*state));
    return -1;
}
