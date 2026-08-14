/**
 * playos_audio.c — System audio state and master volume (ALSA mixer)
 *
 * Exposes system-wide audio information and volume/mute control through the
 * ALSA master playback mixer element. Volume is system-wide: the shell and
 * overlay (trusted, always foreground from their own perspective) are always
 * honored; a game process is honored only while it holds the foreground
 * lifecycle state, enforced via playos_lifecycle_is_foreground().
 *
 * PCM playback itself is owned by the application's audio framework
 * (Raylib/miniaudio in this sprint); this file only manages the shared mixer
 * control surface (master volume / mute).
 *
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 200809L

#include "playos/playos_audio.h"
#include "playos/playos_logging.h"

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Provided by playos_lifecycle.c (internal, not part of the public ABI). */
extern int playos_lifecycle_is_foreground(void);

#define PLAYOS_AUDIO_SAMPLE_RATE    44100
#define PLAYOS_AUDIO_CHANNELS       2
#define PLAYOS_AUDIO_BITS_PER_SAMPLE 16

/* Select the ALSA control device to attach the mixer to. The PCM playback
 * device priority (PLAYOS_AUDIO_DEVICE -> headphone jack -> built-in
 * speakers) is owned by miniaudio's ALSA backend; for the mixer we honor
 * PLAYOS_AUDIO_DEVICE when set, otherwise fall back to "default", which the
 * Ally's ALSA card maps to the active output (jack or speakers). */
static const char *
audio_control_device(char *buf, size_t bufsz)
{
    const char *env = getenv("PLAYOS_AUDIO_DEVICE");
    if (env && env[0]) {
        snprintf(buf, bufsz, "%s", env);
        return buf;
    }
    return "default";
}

/* Open the mixer attached to the preferred control device, register simple
 * mixer elements, and load the card. Returns 0 on success. On failure the
 * mixer is closed and -1 is returned; callers treat this as a graceful
 * "no audio hardware" condition (expected under QEMU/CI). */
static int
audio_mixer_open(snd_mixer_t **out)
{
    *out = NULL;

    snd_mixer_t *mixer = NULL;
    if (snd_mixer_open(&mixer, 0) < 0) {
        PLAYOS_LOG_W("audio", "snd_mixer_open() failed");
        return -1;
    }

    char dev_buf[128];
    const char *device = audio_control_device(dev_buf, sizeof(dev_buf));

    if (snd_mixer_attach(mixer, device) < 0) {
        PLAYOS_LOG_W("audio", "snd_mixer_attach(%s) failed; trying default",
                     device);
        snd_mixer_close(mixer);
        mixer = NULL;
        if (snd_mixer_open(&mixer, 0) < 0) {
            PLAYOS_LOG_W("audio", "snd_mixer_open(default) failed");
            return -1;
        }
        device = "default";
        if (snd_mixer_attach(mixer, device) < 0) {
            PLAYOS_LOG_W("audio", "snd_mixer_attach(default) failed");
            snd_mixer_close(mixer);
            return -1;
        }
    }

    if (snd_mixer_selem_register(mixer, NULL, NULL) < 0) {
        PLAYOS_LOG_W("audio", "snd_mixer_selem_register(%s) failed", device);
        snd_mixer_close(mixer);
        return -1;
    }

    if (snd_mixer_load(mixer) < 0) {
        PLAYOS_LOG_W("audio", "snd_mixer_load(%s) failed", device);
        snd_mixer_close(mixer);
        return -1;
    }

    PLAYOS_LOG_D("audio", "mixer attached to '%s'", device);
    *out = mixer;
    return 0;
}

/* Locate the first simple mixer element that has a playback volume control.
 * Returns NULL when none is found. */
static snd_mixer_elem_t *
audio_find_master(snd_mixer_t *mixer)
{
    if (!mixer)
        return NULL;

    snd_mixer_elem_t *elem = snd_mixer_first_elem(mixer);
    while (elem) {
        if (snd_mixer_selem_has_playback_volume(elem))
            return elem;
        elem = snd_mixer_elem_next(elem);
    }
    return NULL;
}

/* Locate the first simple mixer element that has a playback mute switch.
 * This is often a *different* element from the volume control on Realtek
 * codecs (e.g. "Speaker Playback Volume" vs "Speaker Playback Switch"), so
 * mute state must be read/written independently of the volume element. */
static snd_mixer_elem_t *
audio_find_switch(snd_mixer_t *mixer)
{
    if (!mixer)
        return NULL;

    snd_mixer_elem_t *elem = snd_mixer_first_elem(mixer);
    while (elem) {
        if (snd_mixer_selem_has_playback_switch(elem))
            return elem;
        elem = snd_mixer_elem_next(elem);
    }
    return NULL;
}

/* Cached mixer handle: opened lazily on first use and reused for the life of
 * the process. Opening/closing the ALSA mixer on every call was wasteful (the
 * overlay polls get_info() once per frame) and spammed the log whenever no
 * sound hardware is present (e.g. headless QEMU/CI). The handle stays valid
 * for the process lifetime; element reads/writes always reflect live mixer
 * state, so polling correctness is preserved.
 *
 * g_mixer_state: 0 = uninitialized, 1 = open, -1 = unavailable. This API is
 * called from each process's single main loop (shell, overlay, game), so lazy
 * init is not synchronized; add a lock here if that assumption ever changes.
 */
static snd_mixer_t *g_mixer = NULL;
static int g_mixer_state = 0;

static snd_mixer_t *
audio_mixer_get(void)
{
    if (g_mixer_state == 0) {
        if (audio_mixer_open(&g_mixer) == 0)
            g_mixer_state = 1;
        else
            g_mixer_state = -1;
    }
    return g_mixer;
}

int
playos_audio_get_info(PlayOSAudioInfo *info)
{
    if (!info)
        return -1;

    info->sample_rate     = PLAYOS_AUDIO_SAMPLE_RATE;
    info->channels        = PLAYOS_AUDIO_CHANNELS;
    info->bits_per_sample = PLAYOS_AUDIO_BITS_PER_SAMPLE;
    info->master_volume   = 1.0f;
    info->muted           = 0;

    snd_mixer_t *mixer = audio_mixer_get();
    if (!mixer) {
        /* Headless/QEMU: no sound hardware. Return a valid, populated
         * struct so on-screen info still works; never assert. */
        return 0;
    }

    snd_mixer_elem_t *elem = audio_find_master(mixer);
    if (elem) {
        long min = 0, max = 0;
        if (snd_mixer_selem_get_playback_volume_range(elem, &min, &max) >= 0 &&
            max > min) {
            long vol = 0;
            if (snd_mixer_selem_get_playback_volume(
                    elem, SND_MIXER_SCHN_FRONT_LEFT, &vol) >= 0) {
                float v = (float)(vol - min) / (float)(max - min);
                if (v < 0.0f) v = 0.0f;
                if (v > 1.0f) v = 1.0f;
                info->master_volume = v;
            }
        }

    }

    /* Mute state lives on the switch element (which may be distinct from the
     * volume element on Realtek codecs). Fall back to the volume element if
     * it happens to carry the switch itself. */
    snd_mixer_elem_t *sw_elem = audio_find_switch(mixer);
    if (!sw_elem)
        sw_elem = elem;
    if (sw_elem && snd_mixer_selem_has_playback_switch(sw_elem)) {
        int sw = 0;
        if (snd_mixer_selem_get_playback_switch(
                sw_elem, SND_MIXER_SCHN_FRONT_LEFT, &sw) >= 0) {
            info->muted = (sw == 0) ? 1 : 0;
        }
    }

    return 0;
}

int
playos_audio_set_master_volume(float volume)
{
    if (!playos_lifecycle_is_foreground())
        return -1;

    if (volume < 0.0f)
        volume = 0.0f;
    if (volume > 1.0f)
        volume = 1.0f;

    snd_mixer_t *mixer = audio_mixer_get();
    if (!mixer)
        return -1;

    snd_mixer_elem_t *elem = audio_find_master(mixer);
    if (!elem) {
        PLAYOS_LOG_W("audio", "no master playback element found");
        return -1;
    }

    long min = 0, max = 0;
    if (snd_mixer_selem_get_playback_volume_range(elem, &min, &max) < 0 ||
        max <= min) {
        return -1;
    }

    long target = min + (long)(volume * (float)(max - min) + 0.5f);
    int ret = snd_mixer_selem_set_playback_volume_all(elem, target);

    if (ret < 0) {
        PLAYOS_LOG_W("audio", "set master volume to %.2f failed", volume);
        return -1;
    }

    PLAYOS_LOG_D("audio", "set master volume to %.2f", volume);
    return 0;
}

int
playos_audio_set_muted(int muted)
{
    if (!playos_lifecycle_is_foreground())
        return -1;

    snd_mixer_t *mixer = audio_mixer_get();
    if (!mixer)
        return -1;

    snd_mixer_elem_t *elem = audio_find_switch(mixer);
    if (!elem)
        elem = audio_find_master(mixer);
    if (!elem) {
        PLAYOS_LOG_W("audio", "no master playback element found");
        return -1;
    }

    if (!snd_mixer_selem_has_playback_switch(elem)) {
        PLAYOS_LOG_W("audio", "master element has no mute switch");
        return -1;
    }

    int ret = snd_mixer_selem_set_playback_switch_all(elem, muted ? 0 : 1);

    if (ret < 0) {
        PLAYOS_LOG_W("audio", "set muted=%d failed", muted);
        return -1;
    }

    PLAYOS_LOG_D("audio", "set muted=%d", muted);
    return 0;
}
