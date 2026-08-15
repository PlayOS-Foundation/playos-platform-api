/*
 * tests/test_power.c — Unit test for power/thermal API
 *
 * SPDX-License-Identifier: MIT
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "playos/playos_power.h"

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

    printf("PASS: power API guards and info fill\n");
    return 0;
}
