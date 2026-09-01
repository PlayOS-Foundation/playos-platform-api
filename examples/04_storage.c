/*
 * 04_storage.c — per-game paths + atomic save
 *
 * Build:
 *   gcc -I../include 04_storage.c -o storage_demo -lplayos
 */
#include <playos.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    PLAYOS_LOG_I("storage", "install: %s", playos_storage_get_install_path());
    PLAYOS_LOG_I("storage", "saves:   %s", playos_storage_get_saves_path());
    PLAYOS_LOG_I("storage", "cache:   %s", playos_storage_get_cache_path());
    PLAYOS_LOG_I("storage", "free:    %lld bytes",
                 (long long)playos_storage_free_bytes());

    char path[512];
    snprintf(path, sizeof(path), "%s/save.bin", playos_storage_get_saves_path());

    const char payload[] = "player progress";
    if (playos_storage_atomic_write(path, payload, sizeof(payload)) == 0) {
        PLAYOS_LOG_I("storage", "saved %zu bytes atomically", sizeof(payload));
    }
    return 0;
}
