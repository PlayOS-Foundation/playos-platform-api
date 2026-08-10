/**
 * playos_storage.c — Storage paths and helpers (real implementation)
 *
 * Per-game paths are derived from the PLAYOS_GAME_ID environment variable
 * set by playos-init at game launch. For the shell (no GAME_ID set),
 * playos_storage_get_games_path() provides the library root.
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/statvfs.h>

/* Static buffers — valid for process lifetime */
static char g_install_path[512];
static char g_saves_path[512];
static char g_cache_path[512];
static char g_games_path[128] = "/data/games";
static int  g_paths_initialized = 0;

static void init_paths(void)
{
    if (g_paths_initialized)
        return;
    g_paths_initialized = 1;

    const char *game_id = getenv("PLAYOS_GAME_ID");
    if (!game_id || game_id[0] == '\0')
        return; /* Not a game process — paths remain empty */

    snprintf(g_install_path, sizeof(g_install_path),
             "/data/games/%s", game_id);
    snprintf(g_saves_path, sizeof(g_saves_path),
             "/data/saves/%s", game_id);
    snprintf(g_cache_path, sizeof(g_cache_path),
             "/data/cache/%s", game_id);
}

const char *playos_storage_get_install_path(void)
{
    init_paths();
    return g_install_path[0] ? g_install_path : NULL;
}

const char *playos_storage_get_saves_path(void)
{
    init_paths();
    return g_saves_path[0] ? g_saves_path : NULL;
}

const char *playos_storage_get_cache_path(void)
{
    init_paths();
    return g_cache_path[0] ? g_cache_path : NULL;
}

const char *playos_storage_get_games_path(void)
{
    return g_games_path;
}

int64_t playos_storage_free_bytes(void)
{
    struct statvfs buf;
    if (statvfs("/data", &buf) != 0)
        return -1;
    return (int64_t)buf.f_bsize * (int64_t)buf.f_bavail;
}

int playos_storage_atomic_replace(const char *src_path, const char *dst_path)
{
    if (rename(src_path, dst_path) != 0)
        return -1;
    return 0;
}

int playos_storage_atomic_write(const char *path, const void *data, size_t len)
{
    /* Write to temp file, then atomic rename */
    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%d", path, getpid());

    FILE *f = fopen(tmp_path, "wb");
    if (!f)
        return -1;

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        unlink(tmp_path);
        return -1;
    }

    if (rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        return -1;
    }

    return 0;
}
