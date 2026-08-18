/**
 * playos_system.c — System information (real implementation)
 *
 * Reads system info from /proc, /sys, and environment variables.
 * All returned strings use static buffers valid for the process lifetime.
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos.h"
#include "playos/playos_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

/* Static buffers — valid for process lifetime, caller must not free */
static char g_os_version[64];
static char g_device_model[128];
static char g_cpu_description[256];
static char g_gpu_description[128];
static char g_locale[32];
static int  g_initialized = 0;

/* ── Minimal JSON helpers for boot.json ──────────────────────────────────────
 * boot.json is a fixed, small schema, so a hand-rolled parser is sufficient.
 */

static const char *skip_json_ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;
    return p;
}

/* Extract the quoted string value for a JSON key found in the range
 * [start, end). `key` must already be quoted, e.g. "\"active_slot\"".
 * Returns 1 on success and writes the (unquoted) value into out. */
static int json_string_value(const char *start, const char *end,
                             const char *key, char *out, size_t outsz)
{
    size_t keylen = strlen(key);
    const char *p = start;

    if (outsz == 0)
        return 0;
    out[0] = '\0';

    while (p < end) {
        const char *found = memchr(p, '"', (size_t)(end - p));
        if (!found)
            break;
        if ((size_t)(end - found) >= keylen &&
            strncmp(found, key, keylen) == 0) {
            const char *q = skip_json_ws(found + keylen);
            if (*q != ':') {
                p = found + 1;
                continue;
            }
            q = skip_json_ws(q + 1);
            if (*q != '"') {
                p = found + 1;
                continue;
            }
            q++;
            size_t n = 0;
            while (q < end && *q != '"' && *q != '\0') {
                if (n + 1 < outsz)
                    out[n++] = *q;
                q++;
            }
            out[n] = '\0';
            return 1;
        }
        p = found + 1;
    }

    out[0] = '\0';
    return 0;
}

/* Given a pointer to an opening '{', return a pointer just past its matching
 * '}' (accounting for nested objects and strings), or NULL if unbalanced. */
static const char *json_object_end(const char *open_brace)
{
    int depth = 0;
    int in_string = 0;
    const char *p = open_brace;

    for (; *p; p++) {
        if (in_string) {
            if (*p == '\\') {
                p++;
                if (!*p)
                    break;
                continue;
            }
            if (*p == '"')
                in_string = 0;
            continue;
        }
        if (*p == '"') {
            in_string = 1;
        } else if (*p == '{') {
            depth++;
        } else if (*p == '}') {
            depth--;
            if (depth == 0)
                return p + 1;
        }
    }
    return NULL;
}

static void init_system_info(void)
{
    if (g_initialized)
        return;
    g_initialized = 1;

    /* ── OS version ── */
    {
        snprintf(g_os_version, sizeof(g_os_version), "unknown");

        FILE *f = fopen("/EFI/playos/boot.json", "r");
        if (f) {
            char buf[4096];
            size_t n = fread(buf, 1, sizeof(buf) - 1, f);
            fclose(f);
            buf[n] = '\0';

            char active_slot[8] = {0};
            const char *end = buf + n;
            if (json_string_value(buf, end, "\"active_slot\"",
                                  active_slot, sizeof(active_slot))) {
                const char *slot_key = NULL;
                if (strcmp(active_slot, "a") == 0)
                    slot_key = "\"slot_a\"";
                else if (strcmp(active_slot, "b") == 0)
                    slot_key = "\"slot_b\"";

                if (slot_key) {
                    const char *slot = strstr(buf, slot_key);
                    if (slot && slot < end) {
                        const char *open = strchr(slot, '{');
                        if (open && open < end) {
                            const char *close = json_object_end(open);
                            if (close) {
                                char version[64];
                                if (json_string_value(slot, close, "\"version\"",
                                                      version, sizeof(version)) &&
                                    version[0] != '\0') {
                                    snprintf(g_os_version, sizeof(g_os_version),
                                             "%s", version);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* ── Device model ── */
    {
        FILE *f = fopen("/sys/class/dmi/id/product_name", "r");
        if (f) {
            if (!fgets(g_device_model, sizeof(g_device_model), f))
                g_device_model[0] = '\0';
            fclose(f);
        }
        if (g_device_model[0] != '\0') {
            size_t len = strlen(g_device_model);
            while (len > 0 && (g_device_model[len - 1] == '\n' ||
                   g_device_model[len - 1] == '\r'))
                g_device_model[--len] = '\0';
        } else {
            struct utsname u;
            if (uname(&u) == 0)
                snprintf(g_device_model, sizeof(g_device_model), "%s", u.machine);
            else
                snprintf(g_device_model, sizeof(g_device_model), "Unknown Device");
        }
    }

    /* ── CPU description ── */
    {
        FILE *f = fopen("/proc/cpuinfo", "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "model name", 10) == 0) {
                    char *colon = strchr(line, ':');
                    if (colon) {
                        const char *val = colon + 1;
                        while (*val == ' ' || *val == '\t') val++;
                        size_t vlen = strlen(val);
                        while (vlen > 0 && (val[vlen - 1] == '\n' ||
                               val[vlen - 1] == '\r'))
                            vlen--;
                        size_t n = vlen < sizeof(g_cpu_description) - 1
                                   ? vlen : sizeof(g_cpu_description) - 1;
                        memcpy(g_cpu_description, val, n);
                        g_cpu_description[n] = '\0';
                        break;
                    }
                }
            }
            fclose(f);
        }
        if (g_cpu_description[0] == '\0')
            snprintf(g_cpu_description, sizeof(g_cpu_description), "Unknown CPU");
    }

    /* ── GPU description ── */
    {
        /* Try to read GPU vendor from DRM sysfs — card0 is usually
         * the primary GPU on single-GPU SoC devices like the Ally */
        FILE *f = fopen("/sys/class/drm/card0/device/vendor", "r");
        if (f) {
            char buf[64];
            if (fgets(buf, sizeof(buf), f)) {
                unsigned int vendor_id = 0;
                sscanf(buf, "0x%x", &vendor_id);
                /* Map well-known PCI vendor IDs */
                const char *vendor = "Unknown GPU";
                if (vendor_id == 0x1002) vendor = "AMD";
                else if (vendor_id == 0x10de) vendor = "NVIDIA";
                else if (vendor_id == 0x8086) vendor = "Intel";
                snprintf(g_gpu_description, sizeof(g_gpu_description),
                         "%s GPU", vendor);
            }
            fclose(f);
        }
        /* Try card1 as fallback for dual-GPU systems */
        if (g_gpu_description[0] == '\0') {
            FILE *f2 = fopen("/sys/class/drm/card1/device/vendor", "r");
            if (f2) {
                char buf[64];
                if (fgets(buf, sizeof(buf), f2)) {
                    unsigned int vendor_id = 0;
                    sscanf(buf, "0x%x", &vendor_id);
                    const char *vendor = "Unknown GPU";
                    if (vendor_id == 0x1002) vendor = "AMD";
                    else if (vendor_id == 0x10de) vendor = "NVIDIA";
                    else if (vendor_id == 0x8086) vendor = "Intel";
                    snprintf(g_gpu_description, sizeof(g_gpu_description),
                             "%s GPU", vendor);
                }
                fclose(f2);
            }
        }
        if (g_gpu_description[0] == '\0')
            snprintf(g_gpu_description, sizeof(g_gpu_description), "GPU");
    }

    /* ── Locale ── */
    {
        const char *lang = getenv("LANG");
        if (lang && lang[0])
            snprintf(g_locale, sizeof(g_locale), "%s", lang);
        else
            snprintf(g_locale, sizeof(g_locale), "en-US");
    }
}

uint32_t playos_system_api_version(void)
{
    return PLAYOS_API_VERSION;
}

const char *playos_system_os_version(void)
{
    init_system_info();
    return g_os_version;
}

const char *playos_system_device_model(void)
{
    init_system_info();
    return g_device_model;
}

const char *playos_system_cpu_description(void)
{
    init_system_info();
    return g_cpu_description;
}

const char *playos_system_gpu_description(void)
{
    init_system_info();
    return g_gpu_description;
}

uint64_t playos_system_total_memory_bytes(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    char line[256];
    uint64_t total = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%lu", &total);
            total *= 1024; /* MemTotal is in kB */
            break;
        }
    }
    fclose(f);
    return total;
}

uint64_t playos_system_available_memory_bytes(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    char line[256];
    uint64_t avail = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line + 13, "%lu", &avail);
            avail *= 1024;
            break;
        }
    }
    fclose(f);
    return avail;
}

const char *playos_system_locale(void)
{
    init_system_info();
    return g_locale;
}
