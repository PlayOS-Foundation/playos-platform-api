/**
 * playos_system.c — System information (stub)
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_system.h"
#include <stddef.h>

uint32_t playos_system_api_version(void)
{
    return 0;
}

const char *playos_system_os_version(void)
{
    return NULL;
}

const char *playos_system_device_model(void)
{
    return NULL;
}

const char *playos_system_cpu_description(void)
{
    return NULL;
}

const char *playos_system_gpu_description(void)
{
    return NULL;
}

uint64_t playos_system_total_memory_bytes(void)
{
    return 0;
}

uint64_t playos_system_available_memory_bytes(void)
{
    return 0;
}

const char *playos_system_locale(void)
{
    return NULL;
}
