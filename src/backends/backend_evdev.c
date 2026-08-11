#define _POSIX_C_SOURCE 199309L
/**
 * backend_evdev.c — evdev prototype backend for ROG Ally controller input
 *
 * Maps Linux evdev event codes to PlayOS logical button/axis contract.
 * Targets the ASUS ROG Ally (2023) built-in controller (Xbox-compatible HID).
 *
 * Button mapping (Xbox controller via xpad/hid-asus):
 *   BTN_SOUTH  → A     → PLAYOS_BUTTON_SOUTH
 *   BTN_EAST   → B     → PLAYOS_BUTTON_EAST
 *   BTN_WEST   → X     → PLAYOS_BUTTON_WEST
 *   BTN_NORTH  → Y     → PLAYOS_BUTTON_NORTH
 *   BTN_START  → START
 *   BTN_SELECT → SELECT
 *   BTN_MODE   → Xbox/Guide → PLAYOS_BUTTON_SYSTEM (reserved)
 *   BTN_THUMBL → L3
 *   BTN_THUMBR → R3
 *   BTN_TL     → L1 (left bumper)
 *   BTN_TR     → R1 (right bumper)
 *
 * D-pad:
 *   ABS_HAT0X  (-1=left, 1=right) → DPAD_LEFT/RIGHT
 *   ABS_HAT0Y  (-1=up, 1=down)    → DPAD_UP/DOWN
 *
 * Sticks:
 *   ABS_X  → LEFT_X   [-32768, 32767] → normalized to [-1.0, 1.0]
 *   ABS_Y  → LEFT_Y   [-32768, 32767] → normalized to [-1.0, 1.0]
 *   ABS_RX → RIGHT_X  [-32768, 32767] → normalized to [-1.0, 1.0]
 *   ABS_RY → RIGHT_Y  [-32768, 32767] → normalized to [-1.0, 1.0]
 *
 * Triggers:
 *   ABS_Z  → LEFT_TRIGGER  [0, 255] → normalized to [0.0, 1.0]
 *   ABS_RZ → RIGHT_TRIGGER [0, 255] → normalized to [0.0, 1.0]
 *
 * Ally-specific:
 *   Quick-menu button (Armoury Crate) → PLAYOS_BUTTON_QUICK_MENU (reserved)
 *   Typically BTN_TRIGGER_HAPPY1 (0x2c0) or via hid-asus key
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend_evdev.h"

#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "playos/playos_logging.h"

/* ── Axis calibration ────────────────────────────────────────────── */

/* Xbox-compatible axis ranges (typical) */
#define STICK_MIN    (-32768)
#define STICK_MAX    (32767)
#define TRIGGER_MIN  (0)
#define TRIGGER_MAX  (255)    /* Some drivers use 1023; detect dynamically */

/* Dead zone: ignore stick movement below this fraction */
#define STICK_DEADZONE  0.05f

/* ── Internal state ──────────────────────────────────────────────── */

static int            evdev_fd = -1;
static int            trigger_max = TRIGGER_MAX;  /* Detected trigger range */
static uint64_t       boot_time_us = 0;

/* Current button bitmap (before reserved-mask stripping) */
static playos_button_mask_t current_buttons = 0;

/* Current axis values (raw) */
static int32_t raw_axes[PLAYOS_AXIS_COUNT] = {0};

/* ── Timestamp helpers ───────────────────────────────────────────── */

static uint64_t get_time_us(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000UL + (uint64_t)ts.tv_nsec / 1000UL;
}

/* ── Button lookup table ─────────────────────────────────────────── */

typedef struct {
    uint16_t               evdev_code;
    playos_button_mask_t   button;
} button_map_t;

static const button_map_t BUTTON_MAP[] = {
    { BTN_SOUTH,           PLAYOS_BUTTON_SOUTH       },
    { BTN_EAST,            PLAYOS_BUTTON_EAST        },
    { BTN_WEST,            PLAYOS_BUTTON_WEST        },
    { BTN_NORTH,           PLAYOS_BUTTON_NORTH       },
    { BTN_START,           PLAYOS_BUTTON_START       },
    { BTN_SELECT,          PLAYOS_BUTTON_SELECT      },
    { BTN_MODE,            PLAYOS_BUTTON_SYSTEM      }, /* Xbox button — reserved */
    { BTN_THUMBL,          PLAYOS_BUTTON_L3          },
    { BTN_THUMBR,          PLAYOS_BUTTON_R3          },
    { BTN_TL,              PLAYOS_BUTTON_L1          },
    { BTN_TR,              PLAYOS_BUTTON_R1          },
    { BTN_TRIGGER_HAPPY1,  PLAYOS_BUTTON_QUICK_MENU  }, /* Ally quick-menu — reserved */
    /*
     * D-pad is handled exclusively via ABS_HAT0X/Y below.
     * BTN_DPAD_* events are intentionally NOT mapped here:
     * xpad/hid-asus report both, and ABS_HAT enforces
     * mutually exclusive directions (LEFT clears RIGHT, etc.).
     */
};

/* ── Axis mapping ────────────────────────────────────────────────── */

typedef struct {
    uint16_t    evdev_code;
    int         axis_index;
    int         is_trigger;  /* 0 = stick ([-1,1]), 1 = trigger ([0,1]) */
} axis_map_t;

static const axis_map_t AXIS_MAP[] = {
    { ABS_X,  PLAYOS_AXIS_LEFT_X,  0 },
    { ABS_Y,  PLAYOS_AXIS_LEFT_Y,  0 },
    { ABS_RX, PLAYOS_AXIS_RIGHT_X, 0 },
    { ABS_RY, PLAYOS_AXIS_RIGHT_Y, 0 },
    { ABS_Z,  PLAYOS_AXIS_LEFT_TRIGGER,  1 },
    { ABS_RZ, PLAYOS_AXIS_RIGHT_TRIGGER, 1 },
};

/* ── Normalization ───────────────────────────────────────────────── */

/**
 * Normalize a stick value from [min, max] to [-1.0, 1.0]
 * with dead zone around zero.
 */
static float normalize_stick(int32_t val, int32_t min, int32_t max)
{
    float range = (float)(max - min);
    if (range <= 0.0f) return 0.0f;

    /* Center at 0 */
    float centered = (float)(val - (min + max) / 2);
    float normalized = centered / (range / 2.0f);

    /* Apply dead zone */
    if (normalized > -STICK_DEADZONE && normalized < STICK_DEADZONE)
        return 0.0f;

    return normalized;
}

/**
 * Normalize a trigger value from [min, max] to [0.0, 1.0].
 */
static float normalize_trigger(int32_t val, int32_t min, int32_t max)
{
    float range = (float)(max - min);
    if (range <= 0.0f) return 0.0f;

    float normalized = (float)(val - min) / range;
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    return normalized;
}

/* ── Event processing ────────────────────────────────────────────── */

/* Maximum events to drain per call to prevent unbounded blocking */
#define MAX_EVENTS_PER_CALL 64

/**
 * Drain pending events from the evdev fd and update internal state.
 * Capped at MAX_EVENTS_PER_CALL to guarantee bounded latency.
 */
static void drain_events(void)
{
    if (evdev_fd < 0) return;

    struct input_event ev;
    ssize_t n;
    int count = 0;

    while (count < MAX_EVENTS_PER_CALL
           && (n = read(evdev_fd, &ev, sizeof(ev))) == sizeof(ev)) {
        count++;
        size_t i;

        switch (ev.type) {
        case EV_KEY:
            for (i = 0; i < sizeof(BUTTON_MAP) / sizeof(BUTTON_MAP[0]); i++) {
                if (ev.code == BUTTON_MAP[i].evdev_code) {
                    if (ev.value) {
                        current_buttons |= BUTTON_MAP[i].button;
                    } else {
                        current_buttons &= ~BUTTON_MAP[i].button;
                    }
                    break;
                }
            }
            break;

        case EV_ABS:
            /* Store raw value */
            for (i = 0; i < sizeof(AXIS_MAP) / sizeof(AXIS_MAP[0]); i++) {
                if (ev.code == AXIS_MAP[i].evdev_code) {
                    raw_axes[AXIS_MAP[i].axis_index] = ev.value;
                    break;
                }
            }

            /* D-pad via ABS_HAT */
            if (ev.code == ABS_HAT0X) {
                if (ev.value < 0) {
                    current_buttons |= PLAYOS_BUTTON_DPAD_LEFT;
                    current_buttons &= ~PLAYOS_BUTTON_DPAD_RIGHT;
                } else if (ev.value > 0) {
                    current_buttons |= PLAYOS_BUTTON_DPAD_RIGHT;
                    current_buttons &= ~PLAYOS_BUTTON_DPAD_LEFT;
                } else {
                    current_buttons &= ~(PLAYOS_BUTTON_DPAD_LEFT | PLAYOS_BUTTON_DPAD_RIGHT);
                }
            } else if (ev.code == ABS_HAT0Y) {
                if (ev.value < 0) {
                    current_buttons |= PLAYOS_BUTTON_DPAD_UP;
                    current_buttons &= ~PLAYOS_BUTTON_DPAD_DOWN;
                } else if (ev.value > 0) {
                    current_buttons |= PLAYOS_BUTTON_DPAD_DOWN;
                    current_buttons &= ~PLAYOS_BUTTON_DPAD_UP;
                } else {
                    current_buttons &= ~(PLAYOS_BUTTON_DPAD_UP | PLAYOS_BUTTON_DPAD_DOWN);
                }
            }
            break;

        default:
            break;
        }
    }
}

/* ── Device discovery ────────────────────────────────────────────── */

/**
 * Find and open the controller event device.
 * Returns fd on success, -1 if not found.
 */
static int open_controller(void)
{
    int fd;
    int best_fd = -1;

    /* Scan /dev/input/event* for a joystick device */
    char dev_path[64];
    for (int i = 0; i < 32; i++) {
        snprintf(dev_path, sizeof(dev_path), "/dev/input/event%d", i);
        fd = open(dev_path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        /* Get device name for diagnostics */
        char name[256] = {0};
        ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);

        /* Check if this is a joystick/gamepad */
        unsigned long ev_bits[EV_MAX / 8 + 1] = {0};
        unsigned long abs_bits[ABS_MAX / 8 + 1] = {0};

        if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
            PLAYOS_LOG_D("input", "platform: skip %s (%s): ioctl(EV) failed",
                         name, dev_path);
            close(fd);
            continue;
        }

        /* Must have EV_ABS and EV_KEY */
        int has_abs = !!(ev_bits[EV_ABS / 8] & (1u << (EV_ABS % 8)));
        int has_key = !!(ev_bits[EV_KEY / 8] & (1u << (EV_KEY % 8)));

        if (!has_abs || !has_key) {
            PLAYOS_LOG_D("input", "platform: skip %s (%s): not abs+key "
                         "(abs=%d key=%d)", name, dev_path, has_abs, has_key);
            close(fd);
            continue;
        }

        /* Must have ABS_X, ABS_Y, ABS_RX, ABS_RY (gamepad axes) */
        if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) < 0) {
            PLAYOS_LOG_D("input", "platform: skip %s (%s): ioctl(ABS) failed",
                         name, dev_path);
            close(fd);
            continue;
        }

        int has_abs_x  = !!(abs_bits[ABS_X  / 8] & (1u << (ABS_X  % 8)));
        int has_abs_y  = !!(abs_bits[ABS_Y  / 8] & (1u << (ABS_Y  % 8)));
        int has_abs_rx = !!(abs_bits[ABS_RX / 8] & (1u << (ABS_RX % 8)));
        int has_abs_ry = !!(abs_bits[ABS_RY / 8] & (1u << (ABS_RY % 8)));
        int has_sticks = has_abs_x && has_abs_y && has_abs_rx && has_abs_ry;

        if (!has_sticks) {
            PLAYOS_LOG_D("input", "platform: skip %s (%s): missing stick "
                         "axes (X=%d Y=%d RX=%d RY=%d)",
                         name, dev_path,
                         has_abs_x, has_abs_y, has_abs_rx, has_abs_ry);
            close(fd);
            continue;
        }

        /* Prefer Xbox/Ally controllers by name */
        int is_preferred =
            strstr(name, "Xbox") || strstr(name, "xbox") ||
            strstr(name, "X-Box") ||
            strstr(name, "Microsoft") ||
            strstr(name, "ASUE") ||
            strstr(name, "ASUS") ||
            strstr(name, "ROG Ally") ||
            strstr(name, "Gamepad");

        if (is_preferred) {
            PLAYOS_LOG_I("input", "platform: found gamepad: %s (%s)",
                         name, dev_path);
            /* Detect trigger range from ABS_Z */
            struct input_absinfo abs_info;
            if (ioctl(fd, EVIOCGABS(ABS_Z), &abs_info) == 0) {
                trigger_max = abs_info.maximum;
                PLAYOS_LOG_D("input", "platform: trigger max = %d", trigger_max);
            }
            if (best_fd >= 0) close(best_fd);
            return fd;
        }

        /* Keep the first viable fallback */
        if (best_fd < 0) {
            best_fd = fd;
            PLAYOS_LOG_I("input", "platform: found gamepad (fallback): %s (%s)",
                         name, dev_path);
        } else {
            PLAYOS_LOG_D("input", "platform: ignoring additional gamepad: %s (%s)",
                         name, dev_path);
            close(fd);
        }
    }

    if (best_fd >= 0) {
        struct input_absinfo abs_info;
        if (ioctl(best_fd, EVIOCGABS(ABS_Z), &abs_info) == 0) {
            trigger_max = abs_info.maximum;
        }
    }

    return best_fd;
}

/* ── Public backend API ──────────────────────────────────────────── */

int backend_evdev_controller_connected(void)
{
    if (evdev_fd >= 0) {
        /* Check if fd is still valid */
        if (fcntl(evdev_fd, F_GETFL) >= 0) return 1;
        /* Stale fd — close and re-scan */
        PLAYOS_LOG_W("input", "platform: controller fd stale, re-scanning");
        close(evdev_fd);
        evdev_fd = -1;
    }

    evdev_fd = open_controller();
    if (evdev_fd < 0) {
        PLAYOS_LOG_W("input", "platform: no controller device found "
                     "(scanned /dev/input/event0-31)");
        return 0;
    }

    /* Capture boot time on first connection */
    if (boot_time_us == 0) {
        boot_time_us = get_time_us();
    }

    PLAYOS_LOG_I("input", "platform: controller connected (fd=%d)", evdev_fd);
    return 1;
}

int backend_evdev_get_controller_state(PlayOSControllerState *state)
{
    if (!state) return -1;

    if (evdev_fd < 0) {
        if (!backend_evdev_controller_connected()) return -1;
    }

    /* Drain pending events */
    drain_events();

    /* Fill state */
    state->buttons     = current_buttons;
    state->timestamp_us = get_time_us();

    /* Normalize axes */
    for (size_t i = 0; i < sizeof(AXIS_MAP) / sizeof(AXIS_MAP[0]); i++) {
        int idx = AXIS_MAP[i].axis_index;

        if (AXIS_MAP[i].is_trigger) {
            state->axes[idx] = normalize_trigger(
                raw_axes[idx], TRIGGER_MIN, trigger_max);
        } else {
            state->axes[idx] = normalize_stick(
                raw_axes[idx], STICK_MIN, STICK_MAX);
        }
    }

    return 0;
}
