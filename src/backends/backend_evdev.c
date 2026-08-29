#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
/**
 * backend_evdev.c — evdev prototype backend for ROG Ally controller input
 *
 * Maps Linux evdev event codes to PlayOS logical button/axis contract.
 * Targets the ASUS ROG Ally (2023) built-in controller (Xbox-compatible HID).
 *
 * The Ally exposes its controls across more than one evdev node. The old
 * pre-reset backend opened three of them and this file does the same:
 *
 *   - gamepad node: all four stick axes plus BTN_SOUTH. Carries the standard
 *     face buttons, sticks, triggers, and d-pad.
 *   - home node: BTN_MODE without BTN_SOUTH (the Xbox/Guide button).
 *   - vendor node (hid-asus-ally): KEY_PROG1/KEY_PROG2 and
 *     BTN_TRIGGER_HAPPY1/BTN_TRIGGER_HAPPY2 (Armoury Crate / Command Center)
 *     and the hardware volume keys. This backend reads only the reserved
 *     buttons here; KEY_VOLUMEUP/DOWN are owned by the trusted shell.
 *
 * Button mapping (standard Xbox face buttons):
 *   BTN_SOUTH  → A     → PLAYOS_BUTTON_SOUTH
 *   BTN_EAST   → B     → PLAYOS_BUTTON_EAST
 *   BTN_WEST   → X     → PLAYOS_BUTTON_WEST
 *   BTN_NORTH  → Y     → PLAYOS_BUTTON_NORTH
 *   BTN_START  → START
 *   BTN_SELECT → SELECT
 *   BTN_THUMBL → L3
 *   BTN_THUMBR → R3
 *   BTN_TL     → L1 (left bumper)
 *   BTN_TR     → R1 (right bumper)
 *
 * Reserved buttons (stripped from game-facing snapshots):
 *   BTN_MODE           → PLAYOS_BUTTON_SYSTEM     (Xbox/Guide)
 *   KEY_PROG1          → PLAYOS_BUTTON_SYSTEM     (Armoury Crate / Home)
 *   BTN_TRIGGER_HAPPY1 → PLAYOS_BUTTON_SYSTEM     (Armoury Crate alt)
 *   KEY_PROG2          → PLAYOS_BUTTON_QUICK_MENU (Command Center)
 *   BTN_TRIGGER_HAPPY2 → PLAYOS_BUTTON_QUICK_MENU (Command Center alt)
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
 * SPDX-License-Identifier: MIT
 */

#include "backend_evdev.h"
#include "gamepad_db.h"

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

static int            evdev_fd = -1;              /* Gamepad node */
static int            home_fd = -1;               /* BTN_MODE-only node */
static int            vendor_fd = -1;             /* hid-asus-ally reserved node */
static int            trigger_max = TRIGGER_MAX;  /* Detected trigger range */
static uint64_t       boot_time_us = 0;

/* ROG Ally built-in controller face-button quirk.
 *
 * The Ally's internal controller enumerates as a standard Xbox 360 pad
 * (name "Microsoft X-Box 360 pad", VID 045e:028e) on the SoC's internal USB
 * port (phys "usb-0000:09:00.3-2/input0"), but its face buttons follow the
 * Linux BTN_X/BTN_Y convention (physical X reports BTN_NORTH, physical Y
 * reports BTN_WEST). This swap fixes the Xbox-standard fallback table for
 * that device. When a SDL_GameControllerDB entry matches, the entry already
 * encodes the correct X/Y and this quirk is disabled (g_remap_from_db).
 * Env override: PLAYOS_ROG_ALLY_FACE_SWAP=1 forces the swap, =0 disables it. */
static int g_rog_ally_face_swap = 0;

/* Per-device remap resolved from the SDL_GameControllerDB (Sprint 13.6).
 * Falls back to the Xbox-standard table when the device has no DB entry. */
static struct playos_db_remap g_remap;
static int g_remap_from_db = 0;

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

/* ── Bit-array helpers (raw evdev, not libevdev) ─────────────────── */

#define BITS_PER_LONG  (sizeof(unsigned long) * 8)
#define EVDEV_BITS(x)  (((unsigned long)(x) / BITS_PER_LONG) + 1)
#define TEST_BIT(bit, array) \
    (((array)[(unsigned long)(bit) / BITS_PER_LONG] >> \
      ((unsigned long)(bit) % BITS_PER_LONG)) & 1)

/* Cached capability snapshot for one event device. */
typedef struct {
    char           name[256];
    char           phys[256];
    unsigned long  ev_bits[EVDEV_BITS(EV_MAX)];
    unsigned long  abs_bits[EVDEV_BITS(ABS_MAX)];
    unsigned long  key_bits[EVDEV_BITS(KEY_MAX)];
} evdev_caps_t;

static int evdev_get_caps(int fd, evdev_caps_t *caps)
{
    memset(caps, 0, sizeof(*caps));

    if (ioctl(fd, EVIOCGNAME(sizeof(caps->name) - 1), caps->name) < 0)
        caps->name[0] = '\0';
    if (ioctl(fd, EVIOCGPHYS(sizeof(caps->phys) - 1), caps->phys) < 0)
        caps->phys[0] = '\0';
    if (ioctl(fd, EVIOCGBIT(0, sizeof(caps->ev_bits)), caps->ev_bits) < 0)
        return -1;
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(caps->abs_bits)), caps->abs_bits) < 0)
        return -1;
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(caps->key_bits)), caps->key_bits) < 0)
        return -1;

    return 0;
}

static int caps_has_gamepad_sticks(const evdev_caps_t *caps)
{
    return TEST_BIT(EV_ABS, caps->ev_bits) &&
           TEST_BIT(EV_KEY, caps->ev_bits) &&
           TEST_BIT(ABS_X,  caps->abs_bits) &&
           TEST_BIT(ABS_Y,  caps->abs_bits) &&
           TEST_BIT(ABS_RX, caps->abs_bits) &&
           TEST_BIT(ABS_RY, caps->abs_bits);
}

/* ── Button lookup table (reserved buttons only) ──────────────────── */
/* Standard face/aux buttons are resolved per-device through the
 * SDL_GameControllerDB remap (g_remap); see process_event(). */

typedef struct {
    uint16_t               evdev_code;
    playos_button_mask_t   button;
} button_map_t;

static const button_map_t BUTTON_MAP[] = {
    { BTN_MODE,            PLAYOS_BUTTON_SYSTEM      }, /* Xbox button — reserved */
    { KEY_PROG1,           PLAYOS_BUTTON_SYSTEM      }, /* Armoury Crate / Home */
    { KEY_PROG2,           PLAYOS_BUTTON_QUICK_MENU  }, /* Command Center */
    { BTN_TRIGGER_HAPPY1,  PLAYOS_BUTTON_SYSTEM      }, /* Armoury Crate alt */
    { BTN_TRIGGER_HAPPY2,  PLAYOS_BUTTON_QUICK_MENU  }, /* Command Center alt */
};

/* Logical PlayOS button for each database button slot. */
static const playos_button_mask_t DB_BUTTON_MASKS[PLAYOS_DB_BTN_COUNT] = {
    PLAYOS_BUTTON_SOUTH,   /* A */
    PLAYOS_BUTTON_EAST,    /* B */
    PLAYOS_BUTTON_WEST,    /* X */
    PLAYOS_BUTTON_NORTH,   /* Y */
    PLAYOS_BUTTON_SELECT,
    PLAYOS_BUTTON_START,
    PLAYOS_BUTTON_L3,
    PLAYOS_BUTTON_R3,
    PLAYOS_BUTTON_L1,
    PLAYOS_BUTTON_R1,
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
static void process_event(const struct input_event *ev, int face_swap)
{
    size_t i;

    switch (ev->type) {
    case EV_KEY: {
        uint16_t code = ev->code;
        if (face_swap) {
            if (code == BTN_WEST)
                code = BTN_NORTH;
            else if (code == BTN_NORTH)
                code = BTN_WEST;
        }

        /* Standard buttons through the per-device database remap. */
        for (i = 0; i < PLAYOS_DB_BTN_COUNT; i++) {
            if ((int)code == g_remap.buttons[i]) {
                if (ev->value)
                    current_buttons |= DB_BUTTON_MASKS[i];
                else
                    current_buttons &= ~DB_BUTTON_MASKS[i];
                return;
            }
        }

        /* D-pad as buttons (pads without an ABS_HAT). */
        if (g_remap.dpad_hat_x < 0) {
            if ((int)code == g_remap.dpad_btn_up) {
                if (ev->value) { current_buttons |= PLAYOS_BUTTON_DPAD_UP;
                                 current_buttons &= ~PLAYOS_BUTTON_DPAD_DOWN; }
                else current_buttons &= ~PLAYOS_BUTTON_DPAD_UP;
                return;
            }
            if ((int)code == g_remap.dpad_btn_down) {
                if (ev->value) { current_buttons |= PLAYOS_BUTTON_DPAD_DOWN;
                                 current_buttons &= ~PLAYOS_BUTTON_DPAD_UP; }
                else current_buttons &= ~PLAYOS_BUTTON_DPAD_DOWN;
                return;
            }
            if ((int)code == g_remap.dpad_btn_left) {
                if (ev->value) { current_buttons |= PLAYOS_BUTTON_DPAD_LEFT;
                                 current_buttons &= ~PLAYOS_BUTTON_DPAD_RIGHT; }
                else current_buttons &= ~PLAYOS_BUTTON_DPAD_LEFT;
                return;
            }
            if ((int)code == g_remap.dpad_btn_right) {
                if (ev->value) { current_buttons |= PLAYOS_BUTTON_DPAD_RIGHT;
                                 current_buttons &= ~PLAYOS_BUTTON_DPAD_LEFT; }
                else current_buttons &= ~PLAYOS_BUTTON_DPAD_RIGHT;
                return;
            }
        }

        /* Reserved buttons (system / quick menu) — never given to games. */
        for (i = 0; i < sizeof(BUTTON_MAP) / sizeof(BUTTON_MAP[0]); i++) {
            if (code == BUTTON_MAP[i].evdev_code) {
                if (ev->value)
                    current_buttons |= BUTTON_MAP[i].button;
                else
                    current_buttons &= ~BUTTON_MAP[i].button;
                break;
            }
        }
        break;
    }

    case EV_ABS:
        /* Sticks and triggers through the per-device remap. */
        if (ev->code == g_remap.abs_left_x)
            raw_axes[PLAYOS_AXIS_LEFT_X] = ev->value;
        else if (ev->code == g_remap.abs_left_y)
            raw_axes[PLAYOS_AXIS_LEFT_Y] = ev->value;
        else if (ev->code == g_remap.abs_right_x)
            raw_axes[PLAYOS_AXIS_RIGHT_X] = ev->value;
        else if (ev->code == g_remap.abs_right_y)
            raw_axes[PLAYOS_AXIS_RIGHT_Y] = ev->value;
        else if (ev->code == g_remap.abs_left_trigger)
            raw_axes[PLAYOS_AXIS_LEFT_TRIGGER] = ev->value;
        else if (ev->code == g_remap.abs_right_trigger)
            raw_axes[PLAYOS_AXIS_RIGHT_TRIGGER] = ev->value;

        /* D-pad via ABS_HAT (mutually exclusive directions). */
        if (g_remap.dpad_hat_x >= 0) {
            if (ev->code == g_remap.dpad_hat_x) {
                if (ev->value < 0) {
                    current_buttons |= PLAYOS_BUTTON_DPAD_LEFT;
                    current_buttons &= ~PLAYOS_BUTTON_DPAD_RIGHT;
                } else if (ev->value > 0) {
                    current_buttons |= PLAYOS_BUTTON_DPAD_RIGHT;
                    current_buttons &= ~PLAYOS_BUTTON_DPAD_LEFT;
                } else {
                    current_buttons &= ~(PLAYOS_BUTTON_DPAD_LEFT | PLAYOS_BUTTON_DPAD_RIGHT);
                }
            } else if (ev->code == g_remap.dpad_hat_x + 1) {
                if (ev->value < 0) {
                    current_buttons |= PLAYOS_BUTTON_DPAD_UP;
                    current_buttons &= ~PLAYOS_BUTTON_DPAD_DOWN;
                } else if (ev->value > 0) {
                    current_buttons |= PLAYOS_BUTTON_DPAD_DOWN;
                    current_buttons &= ~PLAYOS_BUTTON_DPAD_UP;
                } else {
                    current_buttons &= ~(PLAYOS_BUTTON_DPAD_UP | PLAYOS_BUTTON_DPAD_DOWN);
                }
            }
        }
        break;

    default:
        break;
    }
}

static void drain_fd(int fd, int face_swap)
{
    if (fd < 0) return;

    struct input_event ev;
    ssize_t n;
    int count = 0;

    while (count < MAX_EVENTS_PER_CALL
           && (n = read(fd, &ev, sizeof(ev))) == sizeof(ev)) {
        count++;
        process_event(&ev, face_swap);
    }
}

static void drain_events(void)
{
    /* The Ally face-swap only patches the Xbox-standard fallback table.
     * When a DB entry matched, the entry already maps X/Y correctly. */
    drain_fd(evdev_fd, g_rog_ally_face_swap && !g_remap_from_db);
    drain_fd(home_fd, 0);
    drain_fd(vendor_fd, 0);
}

/* ── Device discovery ────────────────────────────────────────────── */

/**
 * Find and open the controller event device.
 * Returns fd on success, -1 if not found.
 */
/* ── Device discovery ────────────────────────────────────────────── */

typedef int (*evdev_caps_predicate)(const evdev_caps_t *caps);

static int open_matching_device(const char *what, evdev_caps_predicate pred)
{
    char dev_path[64];
    for (int i = 0; i < 32; i++) {
        snprintf(dev_path, sizeof(dev_path), "/dev/input/event%d", i);
        int fd = open(dev_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;

        evdev_caps_t caps;
        if (evdev_get_caps(fd, &caps) != 0) {
            close(fd);
            continue;
        }

        if (pred(&caps)) {
            PLAYOS_LOG_I("input", "platform: %s: %s (%s)",
                         what, caps.name[0] ? caps.name : "?", dev_path);
            return fd;
        }

        close(fd);
    }

    return -1;
}

static int is_home_node(const evdev_caps_t *caps)
{
    return TEST_BIT(BTN_MODE, caps->key_bits) &&
           !TEST_BIT(BTN_SOUTH, caps->key_bits);
}

static int is_vendor_node(const evdev_caps_t *caps)
{
    int has_reserved_buttons =
        TEST_BIT(KEY_PROG1, caps->key_bits) ||
        TEST_BIT(KEY_PROG2, caps->key_bits) ||
        TEST_BIT(BTN_TRIGGER_HAPPY1, caps->key_bits) ||
        TEST_BIT(BTN_TRIGGER_HAPPY2, caps->key_bits);

    return has_reserved_buttons &&
           !TEST_BIT(BTN_SOUTH, caps->key_bits) &&
           !TEST_BIT(BTN_MODE, caps->key_bits);
}

/**
 * Find and open the controller event device.
 * Returns fd on success, -1 if not found.
 */
static int open_controller(void)
{
    int best_fd = -1;

    /* Scan /dev/input/event* for a joystick device */
    char dev_path[64];
    for (int i = 0; i < 32; i++) {
        snprintf(dev_path, sizeof(dev_path), "/dev/input/event%d", i);
        int fd = open(dev_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;

        evdev_caps_t caps;
        if (evdev_get_caps(fd, &caps) != 0) {
            PLAYOS_LOG_D("input", "platform: skip %s (%s): ioctl failed",
                         caps.name[0] ? caps.name : "?", dev_path);
            close(fd);
            continue;
        }

        if (!caps_has_gamepad_sticks(&caps)) {
            PLAYOS_LOG_D("input", "platform: skip %s (%s): missing gamepad "
                         "sticks/keys", caps.name[0] ? caps.name : "?", dev_path);
            close(fd);
            continue;
        }

        if (!TEST_BIT(BTN_SOUTH, caps.key_bits)) {
            PLAYOS_LOG_D("input", "platform: skip %s (%s): missing BTN_SOUTH",
                         caps.name[0] ? caps.name : "?", dev_path);
            close(fd);
            continue;
        }

        /* Prefer Xbox/Ally/PlayStation controllers by name */
        int is_preferred =
            strstr(caps.name, "Xbox") || strstr(caps.name, "xbox") ||
            strstr(caps.name, "X-Box") ||
            strstr(caps.name, "Microsoft") ||
            strstr(caps.name, "ASUE") ||
            strstr(caps.name, "ASUS") ||
            strstr(caps.name, "ROG Ally") ||
            strstr(caps.name, "Gamepad") ||
            strstr(caps.name, "Sony") ||
            strstr(caps.name, "DualSense") ||
            strstr(caps.name, "DualShock") ||
            strstr(caps.name, "Wireless Controller");

        if (is_preferred) {
            PLAYOS_LOG_I("input", "platform: found gamepad: %s (%s) fd=%d",
                         caps.name, dev_path, fd);
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
            PLAYOS_LOG_I("input", "platform: found gamepad (fallback): %s (%s) fd=%d",
                         caps.name, dev_path, fd);
        } else {
            PLAYOS_LOG_D("input", "platform: ignoring additional gamepad: %s (%s)",
                         caps.name, dev_path);
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

static int open_home_node(void)
{
    return open_matching_device("home node", is_home_node);
}

static int open_vendor_node(void)
{
    return open_matching_device("vendor node", is_vendor_node);
}

/* Detect the ROG Ally face-swap quirk for the opened gamepad fd. */
static int platform_rog_ally_face_swap(const char *name, const char *phys)
{
    const char *env = getenv("PLAYOS_ROG_ALLY_FACE_SWAP");
    if (env && env[0]) {
        if (env[0] == '1' || env[0] == 'y' || env[0] == 'Y')
            return 1;
        if (env[0] == '0' || env[0] == 'n' || env[0] == 'N')
            return 0;
    }
    return strstr(name, "X-Box") != NULL &&
           strncmp(phys, "usb-0000:09:00.3-2", 18) == 0;
}

static void platform_detect_face_swap(int fd)
{
    char name[256] = {0};
    char phys[256] = {0};
    ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
    ioctl(fd, EVIOCGPHYS(sizeof(phys) - 1), phys);
    g_rog_ally_face_swap = platform_rog_ally_face_swap(name, phys);
    PLAYOS_LOG_I("input", "platform: gamepad face-swap quirk %s "
                 "(name='%s' phys='%s')",
                 g_rog_ally_face_swap ? "ON" : "off", name, phys);
}

/* Resolve the SDL_GameControllerDB entry for the opened gamepad and install
 * the per-device remap (Sprint 13.6). Falls back to the Xbox-standard table
 * when the device has no entry. */
static void platform_resolve_remap(int fd)
{
    struct input_id id;
    memset(&id, 0, sizeof(id));
    if (ioctl(fd, EVIOCGID, &id) < 0)
        id.bustype = id.vendor = id.product = id.version = 0;

    struct playos_db_evdev_tables tables;
    int ok = playos_db_build_tables(fd, &tables);

    if (ok == 0 &&
        playos_db_resolve(&tables, id.bustype, id.vendor, id.product,
                          id.version, &g_remap) == 0) {
        g_remap_from_db = 1;
        char name[256] = {0};
        ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
        PLAYOS_LOG_I("input", "platform: gamepad DB entry matched "
                     "(name='%s' bustype=%04x vendor=%04x product=%04x)",
                     name, id.bustype, id.vendor, id.product);
    } else {
        g_remap_from_db = 0;
        playos_db_default_remap(&g_remap);
        char name[256] = {0};
        ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
        PLAYOS_LOG_I("input", "platform: no gamepad DB entry for '%s' "
                     "(bustype=%04x vendor=%04x product=%04x) — Xbox fallback",
                     name, id.bustype, id.vendor, id.product);
    }

    /* Detect trigger range from the resolved left-trigger axis. */
    if (g_remap.abs_left_trigger >= 0) {
        struct input_absinfo abs_info;
        if (ioctl(fd, EVIOCGABS(g_remap.abs_left_trigger), &abs_info) == 0) {
            trigger_max = abs_info.maximum;
            PLAYOS_LOG_D("input", "platform: trigger max = %d", trigger_max);
        }
    }
}

/* ── Public backend API ──────────────────────────────────────────── */

int backend_evdev_controller_connected(void)
{
    /* Full /dev/input/event0-31 scans are expensive. A node that is
     * genuinely absent must not turn each poll into an open+ioctl pass over
     * every event device, so failed discovery retries are throttled. */
    const uint64_t RESCAN_INTERVAL_US = 2000000; /* 2s */
    static uint64_t controller_next_retry_us = 0;
    static uint64_t home_next_retry_us = 0;
    static uint64_t vendor_next_retry_us = 0;

    if (evdev_fd >= 0) {
        /* Check if fd is still valid */
        if (fcntl(evdev_fd, F_GETFL) < 0) {
            /* Stale fd — close and re-scan */
            PLAYOS_LOG_W("input", "platform: controller fd stale, re-scanning");
            close(evdev_fd);
            evdev_fd = -1;
        }
    }

    if (evdev_fd < 0) {
        uint64_t now = get_time_us();
        if (now < controller_next_retry_us)
            return 0;

        evdev_fd = open_controller();
        if (evdev_fd < 0) {
            PLAYOS_LOG_W("input", "platform: no controller device found "
                         "(scanned /dev/input/event0-31)");
            controller_next_retry_us = now + RESCAN_INTERVAL_US;
            return 0;
        }

        platform_detect_face_swap(evdev_fd);
        platform_resolve_remap(evdev_fd);

        /* Capture boot time on first connection */
        if (boot_time_us == 0) {
            boot_time_us = get_time_us();
        }

        PLAYOS_LOG_I("input", "platform: controller connected (fd=%d)",
                     evdev_fd);
    }

    /* Best-effort re-scan of the two reserved-button nodes. These are
     * independent evdev streams on the ROG Ally (home + armoury crate),
     * so a missing node must not mask an otherwise healthy controller. */
    if (home_fd >= 0 && fcntl(home_fd, F_GETFL) < 0) {
        PLAYOS_LOG_W("input", "platform: home node fd stale, re-scanning");
        close(home_fd);
        home_fd = -1;
    }
    if (home_fd < 0) {
        uint64_t now = get_time_us();
        if (now >= home_next_retry_us) {
            home_fd = open_home_node();
            if (home_fd >= 0) {
                PLAYOS_LOG_I("input", "platform: home node connected (fd=%d)",
                             home_fd);
            }
            home_next_retry_us = now + RESCAN_INTERVAL_US;
        }
    }

    if (vendor_fd >= 0 && fcntl(vendor_fd, F_GETFL) < 0) {
        PLAYOS_LOG_W("input", "platform: vendor node fd stale, re-scanning");
        close(vendor_fd);
        vendor_fd = -1;
    }
    if (vendor_fd < 0) {
        uint64_t now = get_time_us();
        if (now >= vendor_next_retry_us) {
            vendor_fd = open_vendor_node();
            if (vendor_fd >= 0) {
                PLAYOS_LOG_I("input", "platform: vendor node connected (fd=%d)",
                             vendor_fd);
            }
            vendor_next_retry_us = now + RESCAN_INTERVAL_US;
        }
    }

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
    state->axes[PLAYOS_AXIS_LEFT_X] =
        normalize_stick(raw_axes[PLAYOS_AXIS_LEFT_X], STICK_MIN, STICK_MAX);
    state->axes[PLAYOS_AXIS_LEFT_Y] =
        normalize_stick(raw_axes[PLAYOS_AXIS_LEFT_Y], STICK_MIN, STICK_MAX);
    state->axes[PLAYOS_AXIS_RIGHT_X] =
        normalize_stick(raw_axes[PLAYOS_AXIS_RIGHT_X], STICK_MIN, STICK_MAX);
    state->axes[PLAYOS_AXIS_RIGHT_Y] =
        normalize_stick(raw_axes[PLAYOS_AXIS_RIGHT_Y], STICK_MIN, STICK_MAX);
    state->axes[PLAYOS_AXIS_LEFT_TRIGGER] =
        normalize_trigger(raw_axes[PLAYOS_AXIS_LEFT_TRIGGER],
                          TRIGGER_MIN, trigger_max);
    state->axes[PLAYOS_AXIS_RIGHT_TRIGGER] =
        normalize_trigger(raw_axes[PLAYOS_AXIS_RIGHT_TRIGGER],
                          TRIGGER_MIN, trigger_max);

    return 0;
}
