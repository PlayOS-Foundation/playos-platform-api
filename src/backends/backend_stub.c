/**
 * backend_stub.c — Host input backend for the desktop shim (Sprint 15, T5).
 *
 * On the device, libplayos reads the controller from evdev (backend_evdev.c).
 * On a developer's host there are three cases, handled without inventing input:
 *
 *   1. A real gamepad is attached and readable -> delegate to backend_evdev.c, the
 *      same code the device uses. Nothing is duplicated.
 *   2. No gamepad, but the host's keyboard is readable -> map a documented set of
 *      keys to controller state so a game is playable on a laptop. A development
 *      affordance, documented in playos-spec/src/sdk-desktop-shim.md.
 *   3. Neither (Windows, or no permission to read /dev/input) -> report "no
 *      controller", and the game uses raylib's own input, which works everywhere.
 *      A shim that synthesised button presses would be worse than one that reports
 *      nothing.
 *
 * Keyboard map:
 *   WASD              left stick      (what games actually read)
 *   Arrows            d-pad           (menus)
 *   Z / X / C / V     South / East / West / North  (A / B / X / Y)
 *   Q / E             L1 / R1
 *   Enter / Backspace Start / Select
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend_stub.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(__linux__)

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "backend_evdev.h"

#define STUB_MAX_KEYS 8          /* keyboards we are willing to poll */

struct stub_button_map { int code; playos_button_mask_t button; };
struct stub_axis_map   { int code; int axis; float value; };

static const struct stub_button_map STUB_BUTTONS[] = {
    { KEY_Z, PLAYOS_BUTTON_SOUTH },        { KEY_X, PLAYOS_BUTTON_EAST },
    { KEY_C, PLAYOS_BUTTON_WEST },         { KEY_V, PLAYOS_BUTTON_NORTH },
    { KEY_Q, PLAYOS_BUTTON_L1 },           { KEY_E, PLAYOS_BUTTON_R1 },
    { KEY_ENTER, PLAYOS_BUTTON_START },    { KEY_BACKSPACE, PLAYOS_BUTTON_SELECT },
    { KEY_UP, PLAYOS_BUTTON_DPAD_UP },     { KEY_DOWN, PLAYOS_BUTTON_DPAD_DOWN },
    { KEY_LEFT, PLAYOS_BUTTON_DPAD_LEFT }, { KEY_RIGHT, PLAYOS_BUTTON_DPAD_RIGHT },
};

static const struct stub_axis_map STUB_AXES[] = {
    { KEY_W, PLAYOS_AXIS_LEFT_Y, -1.0f }, { KEY_S, PLAYOS_AXIS_LEFT_Y, 1.0f },
    { KEY_A, PLAYOS_AXIS_LEFT_X, -1.0f }, { KEY_D, PLAYOS_AXIS_LEFT_X, 1.0f },
};

static int g_kbd[STUB_MAX_KEYS];
static int g_kbd_count = -1;                 /* -1 = not probed yet */
static unsigned char g_pressed[KEY_MAX + 1];

/**
 * Pure mapping: pressed-key bitmap -> controller state.
 *
 * Deliberately separate from the evdev read so it is unit-testable without
 * hardware (tests/test_desktop_shim.c). It recomputes from the bitmap rather than
 * toggling bits per event, so releasing one of two keys that share an axis leaves
 * the other applied.
 *
 * `pressed` is indexed by key code and is `count` bytes long.
 * Returns 1 when anything is held, 0 when nothing is.
 */
int
backend_stub_state_from_keys(const unsigned char *pressed, size_t count,
                             PlayOSControllerState *state)
{
    if (!pressed || !state)
        return 0;

    state->buttons = 0;
    for (int i = 0; i < PLAYOS_AXIS_COUNT; i++)
        state->axes[i] = 0.0f;

    int held = 0;

    for (size_t i = 0; i < sizeof(STUB_BUTTONS) / sizeof(STUB_BUTTONS[0]); i++) {
        int code = STUB_BUTTONS[i].code;
        if (code >= 0 && (size_t)code < count && pressed[code]) {
            state->buttons |= STUB_BUTTONS[i].button;
            held = 1;
        }
    }
    for (size_t i = 0; i < sizeof(STUB_AXES) / sizeof(STUB_AXES[0]); i++) {
        int code = STUB_AXES[i].code;
        if (code >= 0 && (size_t)code < count && pressed[code]) {
            state->axes[STUB_AXES[i].axis] = STUB_AXES[i].value;
            held = 1;
        }
    }
    return held;
}

/* A keyboard has letter keys and no gamepad buttons. Devices that look like pads
 * are left to backend_evdev.c, so we never double-count the same hardware. */
static int
stub_is_keyboard(int fd)
{
    unsigned long keybits[(KEY_MAX / (8 * sizeof(long))) + 1];
    memset(keybits, 0, sizeof(keybits));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) < 0)
        return 0;

#define KEY_BIT_SET(b) \
    (keybits[(b) / (8 * sizeof(long))] & (1UL << ((b) % (8 * sizeof(long)))))
    return KEY_BIT_SET(KEY_A) && KEY_BIT_SET(KEY_SPACE) && !KEY_BIT_SET(BTN_SOUTH);
#undef KEY_BIT_SET
}

static void
stub_probe_keyboards(void)
{
    g_kbd_count = 0;

    for (int i = 0; i < 32 && g_kbd_count < STUB_MAX_KEYS; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;
        if (!stub_is_keyboard(fd)) {
            close(fd);
            continue;
        }
        g_kbd[g_kbd_count++] = fd;
    }

    if (g_kbd_count == 0)
        fprintf(stderr, "[I] playos(host): no readable keyboard; games use raylib input\n");
}

static void
stub_drain_keyboard(int fd)
{
    struct input_event ev;
    ssize_t n;

    while ((n = read(fd, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
        if (ev.type != EV_KEY || ev.code < 0 || ev.code > KEY_MAX)
            continue;
        g_pressed[ev.code] = (ev.value != 0);
    }
}

/* Merge the keyboard into *state. Returns 1 when anything was held. */
static int
stub_poll_keyboard(PlayOSControllerState *state)
{
    if (g_kbd_count < 0)
        stub_probe_keyboards();

    for (int i = 0; i < g_kbd_count; i++)
        stub_drain_keyboard(g_kbd[i]);

    return backend_stub_state_from_keys(g_pressed, sizeof(g_pressed), state);
}

#else /* not Linux: no evdev, so there are no key codes and no controller */

int
backend_stub_state_from_keys(const unsigned char *pressed, size_t count,
                             PlayOSControllerState *state)
{
    (void)pressed;
    (void)count;
    (void)state;
    return 0;
}

static int
stub_poll_keyboard(PlayOSControllerState *state)
{
    (void)state;
    return 0;
}

#endif

static void
stub_stamp(PlayOSControllerState *state)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    state->timestamp_us = (uint64_t)ts.tv_sec * 1000000ull +
                          (uint64_t)(ts.tv_nsec / 1000);
}

int
backend_stub_controller_connected(void)
{
#if defined(__linux__)
    if (backend_evdev_controller_connected())
        return 1;
#endif

    PlayOSControllerState tmp;
    memset(&tmp, 0, sizeof(tmp));
    return stub_poll_keyboard(&tmp);
}

int
backend_stub_get_controller_state(PlayOSControllerState *state)
{
    if (!state)
        return -1;

    memset(state, 0, sizeof(*state));

#if defined(__linux__)
    /* A real gamepad wins outright; the keyboard is the fallback. */
    if (backend_evdev_get_controller_state(state) == 0) {
        stub_stamp(state);
        return 0;
    }
#endif

    if (stub_poll_keyboard(state)) {
        stub_stamp(state);
        return 0;
    }

    return -1;
}
