/**
 * test_desktop_shim.c — Sprint 15, T5 acceptance.
 *
 * The host shim's promise is narrow and testable: a game linked against it can
 * start and run on a Linux host, its lifecycle calls are safe no-ops (they must
 * never block a desktop game), the input mapping works without hardware, and
 * storage points at the host rather than at /data.
 *
 * The one thing this cannot check is the window itself - that needs a display and
 * belongs to T6 (see playos-spec/src/sdk-desktop-shim.md).
 *
 * Usage:
 *   test_desktop_shim            run the assertions (CI / headless)
 *   test_desktop_shim --watch 5  print live controller state for 5 seconds
 *
 * --watch is the hardware check: run it under sudo (to read /dev/input) and press
 * keys or use a gamepad; anything the shim sees is printed as it changes.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "playos/playos.h"
#include "backends/backend_stub.h"

#if defined(__linux__)
#include <linux/input.h>   /* KEY_* codes, for the mapping assertions */
#endif

static double
now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

#define CHECK(cond, what)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("FAIL: %s\n", (what));                                      \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/* ── --watch: the hardware check ───────────────────────────────────────────── */

static int
watch(double seconds)
{
    printf("watching controller state for %.0f s - press keys or use a gamepad\n",
           seconds);
    printf("  (map: WASD stick, arrows d-pad, Z/X/C/V face, Q/E L1/R1, Enter/Backspace)\n");

    double t0 = now_ms();
    uint32_t last_buttons = 0xffffffffu;
    float last_lx = 99.0f, last_ly = 99.0f;
    int reported_none = 0;

    while (now_ms() - t0 < seconds * 1000.0) {
        PlayOSControllerState st;
        memset(&st, 0, sizeof(st));
        int rc = playos_input_get_controller_state(&st);

        if (rc != 0) {
            if (!reported_none) {
                printf("  [no controller readable - the shim reports none rather than\n"
                       "   inventing presses; run as root to read /dev/input]\n");
                reported_none = 1;
            }
        } else if (st.buttons != last_buttons ||
                   st.axes[PLAYOS_AXIS_LEFT_X] != last_lx ||
                   st.axes[PLAYOS_AXIS_LEFT_Y] != last_ly) {
            printf("  buttons=0x%08x  left=(%.2f,%.2f)  right=(%.2f,%.2f)\n",
                   st.buttons,
                   st.axes[PLAYOS_AXIS_LEFT_X], st.axes[PLAYOS_AXIS_LEFT_Y],
                   st.axes[PLAYOS_AXIS_RIGHT_X], st.axes[PLAYOS_AXIS_RIGHT_Y]);
            last_buttons = st.buttons;
            last_lx = st.axes[PLAYOS_AXIS_LEFT_X];
            last_ly = st.axes[PLAYOS_AXIS_LEFT_Y];
        }

        struct timespec nap = { 0, 10 * 1000 * 1000 };   /* 100 Hz */
        nanosleep(&nap, NULL);
    }

    printf("watch finished\n");
    return 0;
}

/* ── the assertions ────────────────────────────────────────────────────────── */

int
main(int argc, char **argv)
{
    if (argc >= 3 && strcmp(argv[1], "--watch") == 0)
        return watch(atof(argv[2]));

    /* Storage first: the getters cache their paths, so the game id has to be set
     * before anything reads them. */
    setenv("PLAYOS_GAME_ID", "shim-test", 1);

    /* 1. Lifecycle must be a no-op, not a block. */
    PlayOSLifecycleEvent ev;
    memset(&ev, 0, sizeof(ev));

    double t0 = now_ms();
    int rc = playos_lifecycle_poll(&ev);
    double dt = now_ms() - t0;
    printf("lifecycle_poll:        ret=%d in %.2f ms\n", rc, dt);
    CHECK(dt < 100.0, "playos_lifecycle_poll() blocked - a desktop game would hang");

    t0 = now_ms();
    rc = playos_lifecycle_wait(&ev, 0);
    dt = now_ms() - t0;
    printf("lifecycle_wait(0):     ret=%d in %.2f ms\n", rc, dt);
    CHECK(dt < 100.0, "playos_lifecycle_wait(0) blocked");

    /* 2. The controller API must answer without inventing input. */
    PlayOSControllerState st;
    memset(&st, 0, sizeof(st));
    t0 = now_ms();
    rc = playos_input_get_controller_state(&st);
    dt = now_ms() - t0;
    printf("controller_state:      ret=%d buttons=0x%08x axes=(%.2f,%.2f) in %.2f ms\n",
           rc, st.buttons, st.axes[PLAYOS_AXIS_LEFT_X], st.axes[PLAYOS_AXIS_LEFT_Y], dt);
    CHECK(rc == 0 || rc == -1, "controller state returned an unexpected value");
    CHECK(dt < 200.0, "playos_input_get_controller_state() blocked");

    /* 3. Storage must point at the host, not at /data, and work. */
    const char *saves = playos_storage_get_saves_path();
    printf("storage_saves_path:    %s\n", saves ? saves : "(null)");
    CHECK(saves != NULL, "no saves path in a game process");
    CHECK(strncmp(saves, "/data", 5) != 0,
          "the shim still hands out /data paths - the storage root seam is not active");

    int64_t freeb = playos_storage_free_bytes();
    printf("storage_free_bytes:    %lld\n", (long long)freeb);
    CHECK(freeb > 0, "free_bytes() must report the host's space, not fail on /data");

    const char *payload = "desktop shim round trip\n";
    char path[512];
    snprintf(path, sizeof(path), "%s/round-trip.txt", saves);

    rc = playos_storage_atomic_write(path, payload, strlen(payload));
    printf("storage_atomic_write:  ret=%d (%s)\n", rc, path);
    CHECK(rc == 0, "playos_storage_atomic_write() failed in the shim's own tree");

    FILE *f = fopen(path, "r");
    CHECK(f != NULL, "the file the shim wrote is not readable");
    char back[64] = {0};
    size_t n = fread(back, 1, sizeof(back) - 1, f);
    fclose(f);
    CHECK(n == strlen(payload) && strcmp(back, payload) == 0,
          "the shim's storage round trip changed the data");
    unlink(path);

    /* 4. Power/profile information must be answerable without playos-init. */
    PlayOSPowerInfo info;
    memset(&info, 0, sizeof(info));
    t0 = now_ms();
    rc = playos_power_get_info(&info);
    dt = now_ms() - t0;
    printf("power_get_info:        ret=%d in %.2f ms\n", rc, dt);
    CHECK(dt < 100.0, "playos_power_get_info() blocked");

#if defined(__linux__)
    /* 5. The key mapping, pure and hardware-free. This is the part of the shim
     *    that is easy to get wrong and impossible to check by reading it. */
    unsigned char keys[KEY_MAX + 1];
    memset(keys, 0, sizeof(keys));
    memset(&st, 0, sizeof(st));

    keys[KEY_W] = 1;
    backend_stub_state_from_keys(keys, sizeof(keys), &st);
    printf("map W:                 left_y=%.2f\n", st.axes[PLAYOS_AXIS_LEFT_Y]);
    CHECK(st.axes[PLAYOS_AXIS_LEFT_Y] == -1.0f, "W must push the left stick up");

    keys[KEY_W] = 0;
    keys[KEY_S] = 1;
    backend_stub_state_from_keys(keys, sizeof(keys), &st);
    CHECK(st.axes[PLAYOS_AXIS_LEFT_Y] == 1.0f, "S must push the left stick down");

    /* Releasing one of two keys that share an axis must leave the other applied:
     * the mapping recomputes from the bitmap instead of toggling per event. */
    keys[KEY_W] = 1;
    keys[KEY_S] = 0;
    keys[KEY_D] = 1;
    backend_stub_state_from_keys(keys, sizeof(keys), &st);
    CHECK(st.axes[PLAYOS_AXIS_LEFT_Y] == -1.0f && st.axes[PLAYOS_AXIS_LEFT_X] == 1.0f,
          "the mapping must survive a shared-axis release");

    memset(keys, 0, sizeof(keys));
    keys[KEY_Z] = 1; keys[KEY_ENTER] = 1; keys[KEY_UP] = 1; keys[KEY_Q] = 1;
    backend_stub_state_from_keys(keys, sizeof(keys), &st);
    printf("map Z+Enter+Up+Q:      buttons=0x%08x\n", st.buttons);
    CHECK((st.buttons & PLAYOS_BUTTON_SOUTH) != 0, "Z must be the south button");
    CHECK((st.buttons & PLAYOS_BUTTON_START) != 0, "Enter must be Start");
    CHECK((st.buttons & PLAYOS_BUTTON_DPAD_UP) != 0, "Up must be the d-pad up");
    CHECK((st.buttons & PLAYOS_BUTTON_L1) != 0, "Q must be L1");
    CHECK(st.axes[PLAYOS_AXIS_LEFT_Y] == 0.0f, "no stick key held means a centred stick");

    memset(keys, 0, sizeof(keys));
    CHECK(backend_stub_state_from_keys(keys, sizeof(keys), &st) == 0,
          "an empty bitmap must report nothing held");
    CHECK(st.buttons == 0, "an empty bitmap must leave the buttons clear");
#endif

    printf("OK - the host shim lets a game run without the device\n");
    return 0;
}
