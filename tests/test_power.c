/*
 * tests/test_power.c — Unit test for power/thermal API
 *
 * SPDX-License-Identifier: MIT
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "playos/playos_power.h"
#include "playos_power_internal.h"

static void test_hwmon_matchers(void)
{
    /* GPU hwmon names: AMD + Intel (Sprint 13 T5) */
    assert(playos__hwmon_name_is_gpu("amdgpu"));
    assert(playos__hwmon_name_is_gpu("i915"));
    assert(playos__hwmon_name_is_gpu("xe"));
    assert(!playos__hwmon_name_is_gpu("k10temp"));
    assert(!playos__hwmon_name_is_gpu("coretemp"));
    assert(!playos__hwmon_name_is_gpu("acpitz"));
    assert(!playos__hwmon_name_is_gpu(NULL));

    /* CPU hwmon names: AMD + Intel (Sprint 13 T5) */
    assert(playos__hwmon_name_is_cpu("k10temp"));
    assert(playos__hwmon_name_is_cpu("coretemp"));
    assert(!playos__hwmon_name_is_cpu("amdgpu"));
    assert(!playos__hwmon_name_is_cpu("i915"));
    assert(!playos__hwmon_name_is_cpu("xe"));
    assert(!playos__hwmon_name_is_cpu(NULL));
}

int main(void)
{
    PlayOSPowerInfo info;

    /* NULL guard */
    assert(playos_power_get_info(NULL) == -1);

    /* Best-effort info fill should succeed (unknown fields use -1) */
    memset(&info, 0xAA, sizeof(info));
    assert(playos_power_get_info(&info) == 0);
    assert(info.battery_percent >= -1 && info.battery_percent <= 100);
    assert(info.minutes_remaining >= -1);
    assert(info.cpu_temp_c >= -1);
    assert(info.gpu_temp_c >= -1);

    /* Invalid profile enum must be denied without touching IPC */
    assert(playos_power_request_profile((PlayOSPerfProfile)99) == -1);

    test_hwmon_matchers();

    printf("PASS: power API guards, info fill, hwmon matchers\n");
    return 0;
}
