/**
 * playos_display.c — Display information (stub)
 *
 * SPDX-License-Identifier: MIT
 */

#include "playos/playos_display.h"
#include <string.h>

int playos_display_get_info(PlayOSDisplayInfo *info)
{
    if (!info) return -1;
    memset(info, 0, sizeof(*info));
    return -1;
}

int playos_display_set_vsync(int enabled)
{
    (void)enabled;
    return -1;
}
