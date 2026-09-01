/*
 * 02_input_and_lifecycle.c — poll lifecycle + read controller state
 *
 * Build:
 *   gcc -I../include 02_input_and_lifecycle.c -o input_loop -lplayos
 */
#include <playos.h>

int main(void)
{
    for (;;) {
        PlayOSLifecycleEvent ev;
        int r = playos_lifecycle_poll(&ev);
        if (r == 1) {
            if (ev == PLAYOS_LIFECYCLE_TERMINATE)
                break;
            if (ev == PLAYOS_LIFECYCLE_BACKGROUND) {
                /* pause, mute, yield CPU */
                continue;
            }
            if (ev == PLAYOS_LIFECYCLE_FOREGROUND) {
                /* resume rendering */
            }
        }

        if (!playos_input_controller_connected())
            continue;

        PlayOSControllerState state;
        if (playos_input_get_controller_state(&state) == 0) {
            if (playos_input_button_down(&state, PLAYOS_BUTTON_SOUTH)) {
                PLAYOS_LOG_I("input", "A button held");
            }
            float lx = state.axes[PLAYOS_AXIS_LEFT_X];
            float ly = state.axes[PLAYOS_AXIS_LEFT_Y];
            if (lx != 0.0f || ly != 0.0f) {
                PLAYOS_LOG_I("input", "left stick (%.2f, %.2f)", lx, ly);
            }
        }
    }
    return 0;
}
