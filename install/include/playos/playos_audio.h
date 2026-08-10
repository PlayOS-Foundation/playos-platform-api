/**
 * playos_audio.h — PlayOS audio state and system volume
 *
 * Games control their own audio streams through their audio framework
 * (e.g. Raylib's audio API). This header exposes system-wide audio
 * information and allows volume/mute control when the game is foreground.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_AUDIO_H
#define PLAYOS_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int   sample_rate;      /**< e.g. 44100 or 48000 */
    int   channels;         /**< 1 = mono, 2 = stereo */
    int   bits_per_sample;  /**< Typically 16 */
    float master_volume;    /**< System master volume [0.0, 1.0] */
    int   muted;            /**< 1 if the system is muted */
} PlayOSAudioInfo;

/**
 * Fills *info with current audio device state.
 *
 * @param[out] info  Destination for audio information.
 * @return  0 on success, -1 on error.
 */
int playos_audio_get_info(PlayOSAudioInfo *info);

/**
 * Request a system master volume change.
 * Only honored when the game is in PLAYOS_LIFECYCLE_FOREGROUND.
 *
 * @param  volume  Desired volume in [0.0, 1.0].
 * @return  0 if accepted, -1 if denied (e.g. game is backgrounded).
 */
int playos_audio_set_master_volume(float volume);

/**
 * Request system mute or unmute.
 * Only honored when the game is in PLAYOS_LIFECYCLE_FOREGROUND.
 *
 * @param  muted  1 to mute, 0 to unmute.
 * @return  0 if accepted, -1 if denied.
 */
int playos_audio_set_muted(int muted);

#ifdef __cplusplus
}
#endif

#endif /* PLAYOS_AUDIO_H */
