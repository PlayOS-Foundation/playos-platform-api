/*
 * 05_power.c — power/thermal info + performance profile
 *
 * Build:
 *   gcc -I../include 05_power.c -o power_demo -lplayos
 */
#include <playos.h>

int main(void)
{
    PlayOSPowerInfo power;
    if (playos_power_get_info(&power) == 0) {
        PLAYOS_LOG_I("power", "state=%d battery=%d%% remaining=%dmin",
                     power.power_state, power.battery_percent,
                     power.minutes_remaining);
        PLAYOS_LOG_I("power", "thermal=%d cpu=%dC gpu=%dC profile=%d",
                     power.thermal_state, power.cpu_temp_c, power.gpu_temp_c,
                     power.active_profile);
    }

    /* Ask for maximum performance (system may deny if hot). */
    if (playos_power_request_profile(PLAYOS_PERF_PERFORMANCE) == 0) {
        PLAYOS_LOG_I("power", "performance profile requested");
    } else {
        PLAYOS_LOG_W("power", "performance profile denied");
    }
    return 0;
}
