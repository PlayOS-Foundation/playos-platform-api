/**
 * test_desktop_shim.c — Sprint 15, T5 acceptance.
 *
 * The host shim's promise is narrow and testable: a game linked against it can
 * start and run on a Linux host, its lifecycle calls are safe no-ops (they must
 * never block a desktop game), and the input API answers without inventing input.
 *
 * The one thing this cannot check is the window itself - that needs a display, and
 * is the developer's own check (see playos-spec/src/sdk-desktop-shim.md).
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "playos/playos.h"

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

int
main(void)
{
    /* 1. Lifecycle must be a no-op, not a block. On the device these talk to
     *    playos-init; here there is nobody to talk to, and a desktop game that
     *    called them must keep running. */
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

    /* 2. The controller API must answer. With no readable input it reports "no
     *    controller" (-1) rather than synthesising one; with a gamepad or a
     *    keyboard it reports state. Either way it must not crash or block. */
    PlayOSControllerState st;
    memset(&st, 0, sizeof(st));
    t0 = now_ms();
    rc = playos_input_get_controller_state(&st);
    dt = now_ms() - t0;
    printf("controller_state:      ret=%d buttons=0x%08x axes=(%.2f,%.2f) in %.2f ms\n",
           rc, st.buttons, st.axes[PLAYOS_AXIS_LEFT_X], st.axes[PLAYOS_AXIS_LEFT_Y], dt);
    CHECK(rc == 0 || rc == -1, "controller state returned an unexpected value");
    CHECK(dt < 200.0, "playos_input_get_controller_state() blocked");

    /* 3. Storage must work: a game that saves on the device should be able to save
     *    on the host too. */
    const char *payload = "desktop shim round trip\n";
    char path[128];
    snprintf(path, sizeof(path), "/tmp/playos-shim-test-%d.txt", (int)getpid());

    rc = playos_storage_atomic_write(path, payload, strlen(payload));
    printf("storage_atomic_write:  ret=%d\n", rc);
    CHECK(rc == 0, "playos_storage_atomic_write() failed on the host");

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

    printf("OK - the host shim lets a game run without the device\n");
    return 0;
}
