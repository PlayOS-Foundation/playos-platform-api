/**
 * playos_power.c — Power, thermal, and performance profiles (stub)
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_power.h"
#include <string.h>

int playos_power_get_info(PlayOSPowerInfo *info)
{
    if (!info) return -1;
    memset(info, 0, sizeof(*info));
    info->battery_percent = -1;
    info->minutes_remaining = -1;
    info->cpu_temp_c = -1;
    info->gpu_temp_c = -1;
    return -1;
}

int playos_power_request_profile(PlayOSPerfProfile profile)
{
    (void)profile;
    return -1;
}
