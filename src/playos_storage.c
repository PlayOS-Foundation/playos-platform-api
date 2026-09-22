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
#include <sys/stat.h>
#include <sys/statvfs.h>

/* Static buffers — valid for process lifetime */
static char g_install_path[512];
static char g_saves_path[512];
static char g_cache_path[512];
static char g_games_path[512];
static int  g_paths_initialized = 0;

/**
 * The storage root, and the single place the two profiles differ.
 *
 * On the device it is /data: playos-init bind-mounts the game's saves, cache and
 * library under it. A desktop has no such tree, so the shim puts the same layout
 * under XDG_DATA_HOME. Everything below is derived from this function so the two
 * profiles cannot drift apart.
 *
 * Sprint 15, T5: without this the desktop profile handed a game "/data/..." paths
 * that do not exist and a free_bytes() that failed, which broke the SDK's promise
 * that the same source runs on a laptop.
 */
static const char *storage_root(void)
{
#ifdef PLAYOS_BACKEND_STUB
    static char root[384];

    if (root[0] != '\0')
        return root;

    const char *base = getenv("XDG_DATA_HOME");
    if (base && base[0] != '\0') {
        snprintf(root, sizeof(root), "%s/playos", base);
    } else {
        const char *home = getenv("HOME");
        if (!home || home[0] == '\0')
            home = "/tmp";
        snprintf(root, sizeof(root), "%s/.local/share/playos", home);
    }
    return root;
#else
    return "/data";
#endif
}

/* mkdir -p, best effort. On the device playos-init has already created (and
 * bind-mounted) these; on the host nobody has, so the shim does. */
static void ensure_dir(const char *path)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s", path);

    for (char *q = buf + 1; *q; q++) {
        if (*q != '/')
            continue;
        *q = '\0';
        (void)mkdir(buf, 0755);
        *q = '/';
    }
    (void)mkdir(buf, 0755);
}

static void init_paths(void)
{
    if (g_paths_initialized)
        return;
    g_paths_initialized = 1;

    const char *root = storage_root();

    /* The library root exists even outside a game (the shell lists games). */
    snprintf(g_games_path, sizeof(g_games_path), "%s/games", root);
    ensure_dir(g_games_path);

    const char *game_id = getenv("PLAYOS_GAME_ID");
    if (!game_id || game_id[0] == '\0')
        return; /* Not a game process — per-game paths remain empty */

    snprintf(g_install_path, sizeof(g_install_path),
             "%s/games/%s", root, game_id);
    snprintf(g_saves_path, sizeof(g_saves_path),
             "%s/saves/%s", root, game_id);
    snprintf(g_cache_path, sizeof(g_cache_path),
             "%s/cache/%s", root, game_id);

    /* A game writes here without asking, so make sure they exist. */
    ensure_dir(g_install_path);
    ensure_dir(g_saves_path);
    ensure_dir(g_cache_path);
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
    init_paths();
    return g_games_path;
}

int64_t playos_storage_free_bytes(void)
{
    struct statvfs buf;
    if (statvfs(storage_root(), &buf) != 0)
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
