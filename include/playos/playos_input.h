/**
 * playos_input.h — PlayOS logical controller input
 *
 * Games receive hardware-agnostic logical input state.
 * PLAYOS_BUTTON_SYSTEM, PLAYOS_BUTTON_QUICK_MENU and PLAYOS_BUTTON_POWER
 * are reserved and are NEVER delivered to game processes.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_INPUT_H
#define PLAYOS_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Button bitmask type. OR multiple flags to test combinations. */
typedef uint32_t playos_button_mask_t;

/** Button bitmask flags. */
enum {
    PLAYOS_BUTTON_SOUTH       = 1u << 0,   /**< A on Xbox layout */
    PLAYOS_BUTTON_EAST        = 1u << 1,   /**< B */
    PLAYOS_BUTTON_WEST        = 1u << 2,   /**< X */
    PLAYOS_BUTTON_NORTH       = 1u << 3,   /**< Y */
    PLAYOS_BUTTON_START       = 1u << 4,
    PLAYOS_BUTTON_SELECT      = 1u << 5,
    PLAYOS_BUTTON_SYSTEM      = 1u << 6,   /**< Xbox/Guide button — RESERVED, not delivered to games */
    PLAYOS_BUTTON_QUICK_MENU  = 1u << 7,   /**< Ally quick-menu button — RESERVED, not delivered to games */
    PLAYOS_BUTTON_DPAD_UP     = 1u << 8,
    PLAYOS_BUTTON_DPAD_DOWN   = 1u << 9,
    PLAYOS_BUTTON_DPAD_LEFT   = 1u << 10,
    PLAYOS_BUTTON_DPAD_RIGHT  = 1u << 11,
    PLAYOS_BUTTON_L1          = 1u << 12,
    PLAYOS_BUTTON_R1          = 1u << 13,
    PLAYOS_BUTTON_L3          = 1u << 14,  /**< Left stick click */
    PLAYOS_BUTTON_R3          = 1u << 15,  /**< Right stick click */
    PLAYOS_BUTTON_POWER       = 1u << 16,  /**< Ally power button — RESERVED, not delivered to games */
};

/** Axis indices into PlayOSControllerState.axes[]. */
typedef enum {
    PLAYOS_AXIS_LEFT_X        = 0,  /**< Left stick horizontal.  Range: [-1.0, 1.0] */
    PLAYOS_AXIS_LEFT_Y        = 1,  /**< Left stick vertical.    Range: [-1.0, 1.0] (up = negative) */
    PLAYOS_AXIS_RIGHT_X       = 2,  /**< Right stick horizontal. Range: [-1.0, 1.0] */
    PLAYOS_AXIS_RIGHT_Y       = 3,  /**< Right stick vertical.   Range: [-1.0, 1.0] */
    PLAYOS_AXIS_LEFT_TRIGGER  = 4,  /**< Left trigger.  Range: [0.0, 1.0] */
    PLAYOS_AXIS_RIGHT_TRIGGER = 5,  /**< Right trigger. Range: [0.0, 1.0] */
    PLAYOS_AXIS_COUNT         = 6
} PlayOSAxis;

/** Snapshot of controller state at a point in time. */
typedef struct {
    playos_button_mask_t buttons;      /**< Bitmask of button flags currently held */
    float                axes[PLAYOS_AXIS_COUNT];  /**< Axis values (see PlayOSAxis for ranges) */
    uint64_t             timestamp_us; /**< Microseconds since system boot */
} PlayOSControllerState;

/**
 * Returns 1 if the primary controller is connected, 0 otherwise.
 */
int playos_input_controller_connected(void);

/**
 * Fills *state with the current controller snapshot.
 *
 * @param[out] state  Destination for the controller snapshot.
 * @return  0 on success, -1 if no controller is connected or on error.
 */
int playos_input_get_controller_state(PlayOSControllerState *state);

/**
 * Convenience helper: returns non-zero if the given button is held.
 */
static inline int playos_input_button_down(const PlayOSControllerState *state,
                                           playos_button_mask_t button)
{
    return (state->buttons & button) != 0u;
}

#ifdef __cplusplus
}
#endif

#endif /* PLAYOS_INPUT_H */
