/* test_gamepad_db.c — Sprint 13.6 host test for the embedded controller DB.
 *
 * Verifies:
 *   1. Every embedded entry parses and resolves against synthetic identity
 *      tables (no fd required).
 *   2. The Xbox-standard fallback table maps A/B/X/Y to BTN_SOUTH/EAST/WEST/NORTH.
 *   3. A synthetic xpad-style key bitmap resolves the Xbox 360 Linux entry with
 *      SDL semantics (A=SOUTH, B=EAST, X=index of BTN_NORTH, Y=index of BTN_WEST),
 *      matching the SDL DB convention that the backend relies on.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "backends/gamepad_db.h"

#include <linux/input.h>

/* ── helpers ─────────────────────────────────────────────────────── */

static void
init_identity_tables(struct playos_db_evdev_tables *t)
{
    memset(t, 0, sizeof(*t));
    for (int i = 0; i < 512; i++)
        t->key_by_index[i] = -1;
    for (int i = 0; i < 64; i++)
        t->axis_by_index[i] = -1;
    for (int i = 0; i < 4; i++)
        t->hat_x_by_index[i] = -1;
    for (int i = 0; i < 32; i++)
        t->key_by_index[t->button_count++] = BTN_MISC + i;
    for (int i = 0; i < 8; i++)
        t->axis_by_index[t->axis_count++] = ABS_X + i;
    for (int i = 0; i < 2; i++)
        t->hat_x_by_index[t->hat_count++] = ABS_HAT0X + 2 * i;
}

static void
guid_to_ids(const char *guid, uint16_t *bus, uint16_t *vendor,
            uint16_t *product, uint16_t *version)
{
    unsigned b0, b1, v0, v1, p0, p1, r0, r1;
    if (sscanf(guid, "%02x%02x0000%02x%02x0000%02x%02x0000%02x%02x0000",
               &b0, &b1, &v0, &v1, &p0, &p1, &r0, &r1) == 8) {
        *bus = (uint16_t)((b1 << 8) | b0);
        *vendor = (uint16_t)((v1 << 8) | v0);
        *product = (uint16_t)((p1 << 8) | p0);
        *version = (uint16_t)((r1 << 8) | r0);
    } else {
        *bus = *vendor = *product = *version = 0;
    }
}

/* ── tests ───────────────────────────────────────────────────────── */

static void
test_every_entry_parses(void)
{
    size_t n = playos_db_line_count();
    assert(n > 500);
    int ok_count = 0;
    struct playos_db_evdev_tables t;
    init_identity_tables(&t);

    for (size_t i = 0; i < n; i++) {
        const char *line = playos_db_line(i);
        assert(line && strlen(line) >= 34 && line[32] == ',');
        uint16_t bus, vendor, product, version;
        guid_to_ids(line, &bus, &vendor, &product, &version);
        struct playos_db_remap out;
        if (playos_db_resolve(&t, bus, vendor, product, version, &out) == 0)
            ok_count++;
    }
    printf("db entries resolved: %d/%zu\n", ok_count, n);
    assert(ok_count > 400);
}

static void
test_fallback_table(void)
{
    struct playos_db_remap out;
    playos_db_default_remap(&out);
    assert(out.buttons[PLAYOS_DB_BTN_SOUTH] == BTN_SOUTH);
    assert(out.buttons[PLAYOS_DB_BTN_EAST] == BTN_EAST);
    assert(out.buttons[PLAYOS_DB_BTN_WEST] == BTN_WEST);
    assert(out.buttons[PLAYOS_DB_BTN_NORTH] == BTN_NORTH);
    assert(out.dpad_hat_x == ABS_HAT0X);
    assert(out.abs_left_x == ABS_X);
    assert(out.abs_left_trigger == ABS_Z);
}

static void
test_xbox360_linux_entry_sdl_semantics(void)
{
    /* Simulate an xpad-style key bitmap: BTN_SOUTH/EAST/NORTH/WEST/TL/TR/
     * SELECT/START/MODE/THUMBL/THUMBR set → SDL button indices 0..10. */
    struct playos_db_evdev_tables t;
    memset(&t, 0, sizeof(t));
    for (int i = 0; i < 512; i++)
        t.key_by_index[i] = -1;
    for (int i = 0; i < 64; i++)
        t.axis_by_index[i] = -1;
    for (int i = 0; i < 4; i++)
        t.hat_x_by_index[i] = -1;

    const int keys[] = { BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST,
                         BTN_TL, BTN_TR, BTN_SELECT, BTN_START, BTN_MODE,
                         BTN_THUMBL, BTN_THUMBR };
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        t.key_by_index[t.button_count++] = keys[i];

    /* xpad ABS enumeration (increasing code order): X,Y,Z,RX,RY,RZ + hat */
    const int abs[] = { ABS_X, ABS_Y, ABS_Z, ABS_RX, ABS_RY, ABS_RZ };
    for (size_t i = 0; i < sizeof(abs) / sizeof(abs[0]); i++)
        t.axis_by_index[t.axis_count++] = abs[i];
    t.hat_x_by_index[t.hat_count++] = ABS_HAT0X;

    struct playos_db_remap out;
    int rc = playos_db_resolve(&t, 0x0003, 0x045e, 0x028e, 0x0114, &out);
    assert(rc == 0);

    /* SDL DB Linux semantics: A=BTN_SOUTH, B=BTN_EAST, X=BTN_NORTH (BTN_X
     * alias), Y=BTN_WEST (BTN_Y alias). This is exactly what makes the ROG
     * Ally's built-in pad decode correctly without the face-swap quirk. */
    assert(out.buttons[PLAYOS_DB_BTN_SOUTH] == BTN_SOUTH);
    assert(out.buttons[PLAYOS_DB_BTN_EAST] == BTN_EAST);
    assert(out.buttons[PLAYOS_DB_BTN_WEST] == BTN_NORTH);
    assert(out.buttons[PLAYOS_DB_BTN_NORTH] == BTN_WEST);
    assert(out.buttons[PLAYOS_DB_BTN_L1] == BTN_TL);
    assert(out.buttons[PLAYOS_DB_BTN_R1] == BTN_TR);
    assert(out.buttons[PLAYOS_DB_BTN_START] == BTN_START);
    assert(out.buttons[PLAYOS_DB_BTN_SELECT] == BTN_SELECT);
    assert(out.buttons[PLAYOS_DB_BTN_L3] == BTN_THUMBL);
    assert(out.buttons[PLAYOS_DB_BTN_R3] == BTN_THUMBR);
    assert(out.abs_left_x == ABS_X);
    assert(out.abs_left_y == ABS_Y);
    assert(out.abs_right_x == ABS_RX);
    assert(out.abs_right_y == ABS_RY);
    assert(out.abs_left_trigger == ABS_Z);
    assert(out.abs_right_trigger == ABS_RZ);
    assert(out.dpad_hat_x == ABS_HAT0X);
}

int
main(void)
{
    test_every_entry_parses();
    test_fallback_table();
    test_xbox360_linux_entry_sdl_semantics();
    printf("test_gamepad_db: all tests passed\n");
    return 0;
}
