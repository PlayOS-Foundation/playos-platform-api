/**
 * playos_power.c — Power, thermal, and performance profiles
 *
 * Reads battery/thermal data from sysfs and requests performance-profile
 * changes from playos-init over the runtime IPC control socket.
 *
 * SPDX-License-Identifier: MIT
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "playos/playos_power.h"
#include "playos/playos_logging.h"

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

/* ── Runtime IPC wire constants (kept in sync with playos-init/ipc/ipc.h) ──
 *
 * We deliberately do NOT vendor ipc.h here. This file only needs the frame
 * layout and one message type for the profile request path.
 */
#define PLAYOS_IPC_MAGIC          0x504C4F53U /* "PLOS" */
#define PLAYOS_IPC_PROTOCOL_VERSION 1
#define PLAYOS_IPC_CONTROL_SOCK   "/run/playos/control.sock"
#define PLAYOS_IPC_TYPE_SET_PERF_PROFILE "SetPerfProfile"

/* ── 1-second result cache (monotonic) ────────────────────────────────────── */

static long long monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static PlayOSPowerInfo g_cached;
static int             g_cached_valid = 0;
static long long       g_cached_ms = 0;

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

/* Read a sysfs text node and strip trailing newline/CR. Returns 0 on success. */
static int read_str_file(const char *path, char *buf, size_t buf_sz)
{
    FILE *f = fopen(path, "r");
    size_t len;

    if (!f)
        return -1;
    if (!fgets(buf, (int)buf_sz, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        buf[--len] = '\0';
    return 0;
}

/* ── Thermal readers ──────────────────────────────────────────────────────── */

/* Read a temperature node in /sys/class/thermal whose "type" matches. */
static int read_thermal_zone_by_type(const char *want)
{
    DIR *d = opendir("/sys/class/thermal");
    struct dirent *e;
    int result = -1;

    if (!d)
        return -1;

    while ((e = readdir(d)) != NULL) {
        char path[288];
        char type[64];

        if (strncmp(e->d_name, "thermal_zone", 12) != 0)
            continue;

        snprintf(path, sizeof(path), "/sys/class/thermal/%s/type", e->d_name);
        if (read_str_file(path, type, sizeof(type)) != 0)
            continue;
        if (strcmp(type, want) != 0)
            continue;

        snprintf(path, sizeof(path), "/sys/class/thermal/%s/temp", e->d_name);
        result = read_int_file(path);
        if (result > 0)
            result /= 1000; /* millidegrees C -> degrees C */
        break;
    }

    closedir(d);
    return result;
}

/* Read the AMD GPU hwmon temp1_input (millidegrees C -> degrees C). */
static int read_hwmon_gpu_temp(void)
{
    DIR *d = opendir("/sys/class/hwmon");
    struct dirent *e;
    int result = -1;

    if (!d)
        return -1;

    while ((e = readdir(d)) != NULL) {
        char path[320];
        char name[64];

        if (strncmp(e->d_name, "hwmon", 5) != 0)
            continue;

        snprintf(path, sizeof(path), "/sys/class/hwmon/%s/name", e->d_name);
        if (read_str_file(path, name, sizeof(name)) != 0)
            continue;
        if (strcmp(name, "amdgpu") != 0)
            continue;

        snprintf(path, sizeof(path), "/sys/class/hwmon/%s/temp1_input", e->d_name);
        result = read_int_file(path);
        if (result > 0)
            result /= 1000;
        break;
    }

    closedir(d);
    return result;
}

/* Read the AMD CPU package temperature from k10temp hwmon. */
static int read_hwmon_cpu_temp(void)
{
    DIR *d = opendir("/sys/class/hwmon");
    struct dirent *e;
    int result = -1;

    if (!d)
        return -1;

    while ((e = readdir(d)) != NULL) {
        char path[320];
        char name[64];

        if (strncmp(e->d_name, "hwmon", 5) != 0)
            continue;

        snprintf(path, sizeof(path), "/sys/class/hwmon/%s/name", e->d_name);
        if (read_str_file(path, name, sizeof(name)) != 0)
            continue;
        if (strcmp(name, "k10temp") != 0)
            continue;

        snprintf(path, sizeof(path), "/sys/class/hwmon/%s/temp1_input", e->d_name);
        result = read_int_file(path);
        if (result > 0)
            result /= 1000;
        break;
    }

    closedir(d);
    return result;
}

static PlayOSThermalState thermal_state_for(int cpu_c, int gpu_c)
{
    int max_c = cpu_c > gpu_c ? cpu_c : gpu_c;

    if (max_c < 0)
        return PLAYOS_THERMAL_NORMAL;
    if (max_c < 75)
        return PLAYOS_THERMAL_NORMAL;
    if (max_c < 85)
        return PLAYOS_THERMAL_WARM;
    if (max_c < 95)
        return PLAYOS_THERMAL_HOT;
    return PLAYOS_THERMAL_CRITICAL;
}

/* ── Battery reader ───────────────────────────────────────────────────────── */

static PlayOSPowerState power_state_for(const char *status)
{
    if (strcmp(status, "Charging") == 0)
        return PLAYOS_POWER_STATE_CHARGING;
    if (strcmp(status, "Full") == 0)
        return PLAYOS_POWER_STATE_CHARGED;
    if (strcmp(status, "Discharging") == 0)
        return PLAYOS_POWER_STATE_ON_BATTERY;
    return PLAYOS_POWER_STATE_UNKNOWN;
}

/* ── Active EPP profile reader ────────────────────────────────────────────── */

static PlayOSPerfProfile read_epp_profile(void)
{
    char buf[64];

    if (read_str_file("/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference",
                      buf, sizeof(buf)) != 0)
        return PLAYOS_PERF_BALANCED;

    if (strcmp(buf, "power") == 0 || strcmp(buf, "balance_power") == 0)
        return PLAYOS_PERF_POWER_SAVE;
    if (strcmp(buf, "performance") == 0)
        return PLAYOS_PERF_PERFORMANCE;
    return PLAYOS_PERF_BALANCED;
}

/* ── Minimal runtime-IPC client for SetPerfProfile ───────────────────────── */

static void put_le32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

static const char *profile_wire_name(PlayOSPerfProfile profile)
{
    switch (profile) {
    case PLAYOS_PERF_POWER_SAVE: return "power_save";
    case PLAYOS_PERF_PERFORMANCE: return "performance";
    case PLAYOS_PERF_BALANCED:
    default:                       return "balanced";
    }
}

/* Returns 0 if the reply contains `"accepted":true`, -1 otherwise. */
static int reply_is_accepted(const unsigned char *buf, size_t n)
{
    static const char needle[] = "\"accepted\":true";

    if (n < sizeof(needle) - 1)
        return -1;
    for (size_t i = 0; i + sizeof(needle) - 1 <= n; i++) {
        if (memcmp(buf + i, needle, sizeof(needle) - 1) == 0)
            return 0;
    }
    return -1;
}

static int request_profile_over_ipc(const char *wire_name)
{
    char body[512];
    int  body_len;
    unsigned char frame[8 + sizeof(body)];
    int  fd;
    struct sockaddr_un addr;
    unsigned char reply[512];
    ssize_t n;

    body_len = snprintf(body, sizeof(body),
                        "{\"v\":%d,\"type\":\"%s\",\"profile\":\"%s\"}",
                        PLAYOS_IPC_PROTOCOL_VERSION,
                        PLAYOS_IPC_TYPE_SET_PERF_PROFILE,
                        wire_name);
    if (body_len < 0 || (size_t)body_len >= sizeof(body))
        return -1;

    put_le32(frame, PLAYOS_IPC_MAGIC);
    put_le32(frame + 4, (uint32_t)body_len);
    memcpy(frame + 8, body, (size_t)body_len);

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", PLAYOS_IPC_CONTROL_SOCK);

    if (connect(fd, (const struct sockaddr *)&addr, (socklen_t)sizeof(addr)) != 0) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }

    if (send(fd, frame, 8 + (size_t)body_len, 0) < 0) {
        close(fd);
        return -1;
    }

    n = recv(fd, reply, sizeof(reply), 0);
    close(fd);

    if (n < 8)
        return -1;
    return reply_is_accepted(reply, (size_t)n);
}

/* ── Public API ───────────────────────────────────────────────────────────── */

int playos_power_get_info(PlayOSPowerInfo *info)
{
    if (!info)
        return -1;

    if (g_cached_valid && (monotonic_ms() - g_cached_ms) < 1000) {
        *info = g_cached;
        return 0;
    }

    memset(info, 0, sizeof(*info));
    info->battery_percent   = -1;
    info->minutes_remaining = -1;
    info->cpu_temp_c        = -1;
    info->gpu_temp_c        = -1;
    info->power_state       = PLAYOS_POWER_STATE_UNKNOWN;
    info->thermal_state     = PLAYOS_THERMAL_NORMAL;
    info->active_profile    = PLAYOS_PERF_BALANCED;

    /* Battery */
    info->battery_percent = read_int_file("/sys/class/power_supply/BAT0/capacity");

    {
        char status[64];
        if (read_str_file("/sys/class/power_supply/BAT0/status",
                          status, sizeof(status)) == 0) {
            info->power_state = power_state_for(status);

            if (info->power_state == PLAYOS_POWER_STATE_ON_BATTERY)
                info->minutes_remaining =
                    read_int_file("/sys/class/power_supply/BAT0/time_to_empty_now");
            else if (info->power_state == PLAYOS_POWER_STATE_CHARGING)
                info->minutes_remaining =
                    read_int_file("/sys/class/power_supply/BAT0/time_to_full_now");
        }
    }

    /* Thermal */
    info->cpu_temp_c = read_thermal_zone_by_type("x86_pkg_temp");
    if (info->cpu_temp_c < 0)
        info->cpu_temp_c = read_thermal_zone_by_type("cpu_thermal");
    if (info->cpu_temp_c < 0)
        info->cpu_temp_c = read_hwmon_cpu_temp();
    info->gpu_temp_c = read_hwmon_gpu_temp();
    info->thermal_state = thermal_state_for(info->cpu_temp_c, info->gpu_temp_c);

    /* Active profile */
    info->active_profile = read_epp_profile();

    g_cached = *info;
    g_cached_valid = 1;
    g_cached_ms = monotonic_ms();
    return 0;
}

int playos_power_request_profile(PlayOSPerfProfile profile)
{
    if (profile != PLAYOS_PERF_BALANCED &&
        profile != PLAYOS_PERF_POWER_SAVE &&
        profile != PLAYOS_PERF_PERFORMANCE) {
        PLAYOS_LOG_W("power", "invalid profile enum %d", (int)profile);
        return -1;
    }

    if (request_profile_over_ipc(profile_wire_name(profile)) != 0) {
        PLAYOS_LOG_W("power", "profile request denied: %s",
                     profile_wire_name(profile));
        return -1;
    }

    PLAYOS_LOG_I("power", "profile request accepted: %s",
                 profile_wire_name(profile));
    return 0;
}
