/**
 * playos_display.c — Display information and backlight control
 *
 * get_info()/set_vsync() remain stubs (the compositor owns display policy);
 * this file now also implements sysfs backlight read/write for the trusted
 * shell's Settings -> Display brightness gauge.
 *
 * SPDX-License-Identifier: MIT
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "playos/playos_display.h"
#include "playos/playos_logging.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ── 1-second result cache (monotonic) ────────────────────────────────────── */

static long long monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static int read_int_file(const char *path)
{
    FILE *f = fopen(path, "r");
    int   v = -1;

    if (!f)
        return -1;
    if (fscanf(f, "%d", &v) != 1)
        v = -1;
    fclose(f);
    return v;
}

/* ── Backlight node discovery ────────────────────────────────────────────── */

static char g_backlight_path[288]; /* e.g. /sys/class/backlight/amdgpu_bl0 */
static int  g_backlight_max = 0;
static int  g_backlight_searched = 0;

static int g_brightness_cached = -1;
static int g_brightness_valid = 0;
static long long g_brightness_ms = 0;

static int g_write_error_logged = 0;

/* Preference order: amdgpu_bl0 -> acpi_video0 -> intel_backlight -> first
 * other non-acpi_ entry. Other acpi_* nodes are deprioritized. */
static int backlight_score(const char *name)
{
    if (strcmp(name, "amdgpu_bl0") == 0)
        return 3;
    if (strcmp(name, "acpi_video0") == 0)
        return 2;
    if (strcmp(name, "intel_backlight") == 0)
        return 1;
    if (strncmp(name, "acpi_", 5) == 0)
        return -1;
    return 0;
}

static int backlight_discover(void)
{
    DIR *d;
    struct dirent *e;
    int best_score = -1;
    char best_path[288] = "";
    int best_max = 0;

    if (g_backlight_searched)
        return (g_backlight_path[0] != '\0') ? 0 : -1;

    g_backlight_searched = 1;

    d = opendir("/sys/class/backlight");
    if (!d)
        return -1;

    while ((e = readdir(d)) != NULL) {
        char path[320];
        int max;
        int score;

        if (e->d_name[0] == '.')
            continue;

        score = backlight_score(e->d_name);
        if (score < 0)
            continue;

        snprintf(path, sizeof(path),
                 "/sys/class/backlight/%s/max_brightness", e->d_name);
        max = read_int_file(path);
        if (max <= 0)
            continue;

        if (score > best_score) {
            best_score = score;
            snprintf(best_path, sizeof(best_path),
                     "/sys/class/backlight/%s", e->d_name);
            best_max = max;
        }
    }

    closedir(d);

    if (best_path[0] == '\0')
        return -1;

    snprintf(g_backlight_path, sizeof(g_backlight_path), "%s", best_path);
    g_backlight_max = best_max;
    PLAYOS_LOG_I("display", "backlight node %s (max %d)",
                 g_backlight_path, g_backlight_max);
    return 0;
}

/* ── Public API ───────────────────────────────────────────────────────────── */

int playos_display_get_info(PlayOSDisplayInfo *info)
{
    if (!info) return -1;
    memset(info, 0, sizeof(*info));
    return -1;
}

int playos_display_set_vsync(int enabled)
{
    (void)enabled;
    return -1;
}

int playos_display_get_brightness(int *percent)
{
    char path[320];
    int raw;
    int pct;

    if (!percent)
        return -1;

    if (backlight_discover() != 0) {
        *percent = -1;
        return -1;
    }

    if (g_brightness_valid && (monotonic_ms() - g_brightness_ms) < 1000) {
        *percent = g_brightness_cached;
        return 0;
    }

    snprintf(path, sizeof(path), "%s/brightness", g_backlight_path);
    raw = read_int_file(path);
    if (raw < 0) {
        g_brightness_valid = 0;
        *percent = -1;
        return -1;
    }

    pct = (raw * 100 + g_backlight_max / 2) / g_backlight_max;
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;

    g_brightness_cached = pct;
    g_brightness_valid = 1;
    g_brightness_ms = monotonic_ms();
    *percent = pct;
    return 0;
}

int playos_display_set_brightness(int percent)
{
    char path[320];
    int raw;
    FILE *f;

    if (backlight_discover() != 0)
        return -1;

    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    raw = (percent * g_backlight_max + 50) / 100;
    if (raw < 0)
        raw = 0;
    if (raw > g_backlight_max)
        raw = g_backlight_max;

    snprintf(path, sizeof(path), "%s/brightness", g_backlight_path);

    f = fopen(path, "w");
    if (!f) {
        if (!g_write_error_logged) {
            PLAYOS_LOG_W("display", "cannot open %s for write: %s",
                         path, strerror(errno));
            g_write_error_logged = 1;
        }
        return -1;
    }

    if (fprintf(f, "%d\n", raw) < 0) {
        int saved = errno;
        fclose(f);
        if (!g_write_error_logged) {
            PLAYOS_LOG_W("display", "write failed to %s: %s",
                         path, strerror(saved));
            g_write_error_logged = 1;
        }
        return -1;
    }

    if (fclose(f) != 0) {
        if (!g_write_error_logged) {
            PLAYOS_LOG_W("display", "close failed on %s: %s",
                         path, strerror(errno));
            g_write_error_logged = 1;
        }
        return -1;
    }

    g_write_error_logged = 0;
    g_brightness_valid = 0; /* force a fresh read on the next get */
    PLAYOS_LOG_I("display", "brightness set to %d%% (raw %d)", percent, raw);
    return 0;
}
