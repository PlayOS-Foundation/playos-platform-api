/**
 * playos.h — PlayOS Platform API master include
 *
 * Include this header to pull in all PlayOS API groups.
 * SPDX-License-Identifier: MIT
 */

#ifndef PLAYOS_H
#define PLAYOS_H

#define PLAYOS_API_VERSION_MAJOR  0
#define PLAYOS_API_VERSION_MINOR  3
#define PLAYOS_API_VERSION_PATCH  0

/** Integer ABI version. Increment on breaking changes. */
#define PLAYOS_API_VERSION  2

#include "playos_system.h"
#include "playos_lifecycle.h"
#include "playos_input.h"
#include "playos_display.h"
#include "playos_storage.h"
#include "playos_audio.h"
#include "playos_power.h"
#include "playos_logging.h"

#endif /* PLAYOS_H */
