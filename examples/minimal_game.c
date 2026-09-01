/*
 * minimal_game.c — the "Getting Started" minimal game
 *
 * A complete (tiny) PlayOS game loop: lifecycle-aware, controller input,
 * atomic save on exit. Render code is omitted for brevity.
 *
 * Build (device):
 *   gcc $(pkg-config --cflags --libs playos) minimal_game.c -o bin/game
 */
#include <playos.h>
#include <stdio.h>
#include <string.h>

static void save_progress(int level)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/save.bin",
             playos_storage_get_saves_path());
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d", level);
    if (playos_storage_atomic_write(path, buf, (size_t)len) == 0) {
        PLAYOS_LOG_I("game", "saved level %d", level);
    }
}

int main(void)
{
    int level = 1;
    int running = 1;

    PLAYOS_LOG_I("game", "starting on %s", playos_system_device_model());

    while (running) {
        /* 1. Lifecycle — must be handled every frame. */
        PlayOSLifecycleEvent ev;
        int r = playos_lifecycle_poll(&ev);
        if (r == 1) {
            switch (ev) {
            case PLAYOS_LIFECYCLE_FOREGROUND:
                PLAYOS_LOG_I("game", "foreground");
                break;
            case PLAYOS_LIFECYCLE_BACKGROUND:
                /* Pause simulation, mute audio, yield CPU. */
                PLAYOS_LOG_I("game", "background");
                break;
            case PLAYOS_LIFECYCLE_SUSPEND:
            case PLAYOS_LIFECYCLE_RESUME:
                break;
            case PLAYOS_LIFECYCLE_TERMINATE:
                save_progress(level);
                running = 0;
                break;
            }
        }

        /* 2. Input. */
        if (playos_input_controller_connected()) {
            PlayOSControllerState state;
            if (playos_input_get_controller_state(&state) == 0) {
                if (playos_input_button_down(&state, PLAYOS_BUTTON_NORTH)) {
                    level++;
                    PLAYOS_LOG_I("game", "level %d", level);
                }
            }
        }

        /* 3. Update + render (omitted): draw at 60fps using raylib. */
    }

    PLAYOS_LOG_I("game", "bye");
    return 0;
}
