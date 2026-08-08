/**
 * playos_audio.c — Audio state and volume (stub)
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_audio.h"
#include <string.h>

int playos_audio_get_info(PlayOSAudioInfo *info)
{
    if (!info) return -1;
    memset(info, 0, sizeof(*info));
    return -1;
}

int playos_audio_set_master_volume(float volume)
{
    (void)volume;
    return -1;
}

int playos_audio_set_muted(int muted)
{
    (void)muted;
    return -1;
}
