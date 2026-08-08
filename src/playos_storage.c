/**
 * playos_storage.c — Storage paths and helpers (stub)
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_storage.h"

const char *playos_storage_get_install_path(void)
{
    return NULL;
}

const char *playos_storage_get_saves_path(void)
{
    return NULL;
}

const char *playos_storage_get_cache_path(void)
{
    return NULL;
}

int64_t playos_storage_free_bytes(void)
{
    return -1;
}

int playos_storage_atomic_replace(const char *src_path, const char *dst_path)
{
    (void)src_path;
    (void)dst_path;
    return -1;
}

int playos_storage_atomic_write(const char *path, const void *data, size_t len)
{
    (void)path;
    (void)data;
    (void)len;
    return -1;
}
