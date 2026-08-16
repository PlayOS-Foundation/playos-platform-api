/**
 * playos_display.h — PlayOS display information
 *
 * Read-only. Games do not configure the display — the compositor owns
 * all display policy. Games may query display properties for layout
 * and rendering decisions.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_DISPLAY_H
#define PLAYOS_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int     width;         /**< Native display width in pixels (e.g. 1920) */
    int     height;        /**< Native display height in pixels (e.g. 1080) */
    float   refresh_rate;  /**< Refresh rate in Hz (e.g. 60.0, 120.0) */
    float   scale;         /**< Logical scale factor (1.0 = 1:1 pixel mapping) */
    int     orientation;   /**< 0 = landscape, 1 = portrait */
    int     hdr_supported; /**< 1 if HDR output is available (post-MVP) */
} PlayOSDisplayInfo;

/**
 * Fills *info with current display properties.
 *
 * @param[out] info  Destination for display information.
 * @return  0 on success, -1 on error.
 */
int playos_display_get_info(PlayOSDisplayInfo *info);

/**
 * Request v-sync preference. PlayOS may or may not honor the request
 * depending on compositor policy.
 *
 * @param  enabled  1 to request v-sync (default), 0 to request uncapped.
 * @return  0 if the request was accepted, -1 if denied or unsupported.
 */
int playos_display_set_vsync(int enabled);

/**
 * Read the current display backlight brightness.
 *
 * @param[out] percent  Current brightness as 0..100, or -1 if unsupported.
 * @return  0 on success, -1 if no backlight control is available.
 */
int playos_display_get_brightness(int *percent);

/**
 * Set the display backlight brightness.
 *
 * @param  percent  Target brightness, 0..100 (clamped).
 * @return  0 on success, -1 if no backlight control is available or the
 *          write failed.
 */
int playos_display_set_brightness(int percent);

#ifdef __cplusplus
}
#endif

#endif /* PLAYOS_DISPLAY_H */
