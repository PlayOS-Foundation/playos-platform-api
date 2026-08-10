/**
 * playos_storage.h — PlayOS per-game storage paths and helpers
 *
 * All path functions return strings valid for the process lifetime.
 * Never free() the returned pointers.
 *
 * Per-game paths are derived from PLAYOS_GAME_ID set by playos-init at launch.
 * Games are isolated to their own save and cache directories — they cannot
 * construct or access another game's paths through this API.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_STORAGE_H
#define PLAYOS_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read-only path to the game's installation directory.
 * e.g. /data/games/com.example.game
 *
 * @return  Null-terminated path string, or NULL if unavailable.
 */
const char *playos_storage_get_install_path(void);

/**
 * Read-write path to the game's save data directory.
 * e.g. /data/saves/com.example.game
 * The directory is created by playos-init before the game is launched.
 *
 * @return  Null-terminated path string, or NULL if unavailable.
 */
const char *playos_storage_get_saves_path(void);

/**
 * Read-write path to the game's cache directory.
 * e.g. /data/cache/com.example.game
 * Cache data may be cleared by the user at any time. Do not store
 * save data or user progress here.
 *
 * @return  Null-terminated path string, or NULL if unavailable.
 */
const char *playos_storage_get_cache_path(void);

/**
 * Returns the free space on the data partition in bytes.
 * Returns -1 on error.
 */
int64_t playos_storage_free_bytes(void);

/**
 * Atomically replace dst_path with src_path using a filesystem rename.
 * Ensures that dst_path is never left in a partial state.
 *
 * Typical use: write to a temp file, then call this to move it into place.
 *
 * @param  src_path  Source file path (will be renamed/moved).
 * @param  dst_path  Destination file path.
 * @return  0 on success, -1 on error (check errno).
 */
int playos_storage_atomic_replace(const char *src_path, const char *dst_path);

/**
 * Write data to a temporary file and atomically rename it to path.
 * The write is safe against crashes — either the old file or the
 * complete new file will be present after a crash.
 *
 * @param  path  Destination file path.
 * @param  data  Data to write.
 * @param  len   Number of bytes to write.
 * @return  0 on success, -1 on error (check errno).
 */
int playos_storage_atomic_write(const char *path, const void *data, size_t len);

/**
 * Root directory for all installed games.
 * e.g. /data/games
 *
 * This is always available regardless of whether PLAYOS_GAME_ID is set,
 * making it suitable for use by the shell for game library discovery.
 *
 * @return  Null-terminated path string. Always valid.
 */
const char *playos_storage_get_games_path(void);

#ifdef __cplusplus
}
#endif

#endif /* PLAYOS_STORAGE_H */
