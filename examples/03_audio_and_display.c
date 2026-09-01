/*
 * 03_audio_and_display.c — audio state/volume + display info/brightness
 *
 * Build:
 *   gcc -I../include 03_audio_and_display.c -o av_info -lplayos
 */
#include <playos.h>

int main(void)
{
    PlayOSAudioInfo audio;
    if (playos_audio_get_info(&audio) == 0) {
        PLAYOS_LOG_I("av", "audio %dHz %dch %dbit vol=%.2f muted=%d",
                     audio.sample_rate, audio.channels, audio.bits_per_sample,
                     audio.master_volume, audio.muted);
        playos_audio_set_master_volume(0.8f);
        playos_audio_set_muted(0);
    }

    PlayOSDisplayInfo display;
    if (playos_display_get_info(&display) == 0) {
        PLAYOS_LOG_I("av", "display %dx%d @%.1fHz scale=%.1f",
                     display.width, display.height, display.refresh_rate,
                     display.scale);
    }

    int brightness = 0;
    if (playos_display_get_brightness(&brightness) == 0) {
        PLAYOS_LOG_I("av", "brightness %d%%", brightness);
        playos_display_set_brightness(70);
    }
    return 0;
}
