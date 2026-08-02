/**
 * playos_system.h — PlayOS system and device information
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_SYSTEM_H
#define PLAYOS_SYSTEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the integer API version this runtime implements.
 * Compare against PLAYOS_API_VERSION to check compatibility.
 */
uint32_t    playos_system_api_version(void);

/**
 * Null-terminated OS version string, e.g. "0.1.0".
 * Valid for the process lifetime. Never free().
 */
const char *playos_system_os_version(void);

/**
 * Null-terminated device model string, e.g. "ROG Ally (2023)".
 * Valid for the process lifetime. Never free().
 */
const char *playos_system_device_model(void);

/**
 * Null-terminated CPU description, e.g. "AMD Ryzen Z1 Extreme".
 * Valid for the process lifetime. Never free().
 */
const char *playos_system_cpu_description(void);

/**
 * Null-terminated GPU description, e.g. "AMD Radeon 780M (RDNA 3)".
 * Valid for the process lifetime. Never free().
 */
const char *playos_system_gpu_description(void);

/** Total installed RAM in bytes. */
uint64_t    playos_system_total_memory_bytes(void);

/** Currently available RAM in bytes. */
uint64_t    playos_system_available_memory_bytes(void);

/**
 * BCP 47 locale string, e.g. "en-US".
 * Valid for the process lifetime. Never free().
 */
const char *playos_system_locale(void);

#ifdef __cplusplus
}
#endif

#endif /* PLAYOS_SYSTEM_H */
