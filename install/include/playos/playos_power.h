/**
 * playos_power.h — PlayOS battery, thermal, and performance profiles
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_POWER_H
#define PLAYOS_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLAYOS_POWER_STATE_ON_BATTERY, /**< Running on battery */
    PLAYOS_POWER_STATE_CHARGING,   /**< Charging via AC/USB-C */
    PLAYOS_POWER_STATE_CHARGED,    /**< Fully charged and on AC */
    PLAYOS_POWER_STATE_UNKNOWN     /**< State cannot be determined */
} PlayOSPowerState;

typedef enum {
    PLAYOS_THERMAL_NORMAL,   /**< < 75°C  — normal operation */
    PLAYOS_THERMAL_WARM,     /**< 75–85°C — elevated, monitor */
    PLAYOS_THERMAL_HOT,      /**< 85–95°C — system may reduce performance */
    PLAYOS_THERMAL_CRITICAL  /**< ≥ 95°C  — system will shut down */
} PlayOSThermalState;

typedef enum {
    PLAYOS_PERF_BALANCED,     /**< Default — system-managed, balanced power/performance */
    PLAYOS_PERF_POWER_SAVE,   /**< Low TDP — maximizes battery life */
    PLAYOS_PERF_PERFORMANCE   /**< High TDP — maximizes CPU/GPU performance */
} PlayOSPerfProfile;

typedef struct {
    PlayOSPowerState   power_state;
    int                battery_percent;    /**< 0–100. -1 if unknown. */
    int                minutes_remaining;  /**< Estimated minutes until empty. -1 if unknown or charging. */
    PlayOSThermalState thermal_state;
    int                cpu_temp_c;         /**< CPU package temperature in °C. -1 if unknown. */
    int                gpu_temp_c;         /**< GPU temperature in °C. -1 if unknown. */
    PlayOSPerfProfile  active_profile;     /**< Currently active performance profile. */
} PlayOSPowerInfo;

/**
 * Fills *info with current power and thermal state.
 *
 * @param[out] info  Destination for power information.
 * @return  0 on success, -1 on error.
 */
int playos_power_get_info(PlayOSPowerInfo *info);

/**
 * Request a performance profile change.
 *
 * PlayOS may deny or override the request based on thermal state.
 * For example, PLAYOS_PERF_PERFORMANCE will be denied when
 * thermal_state >= PLAYOS_THERMAL_HOT.
 *
 * @param  profile  Desired performance profile.
 * @return  0 if the request was accepted, -1 if denied.
 */
int playos_power_request_profile(PlayOSPerfProfile profile);

#ifdef __cplusplus
}
#endif

#endif /* PLAYOS_POWER_H */
