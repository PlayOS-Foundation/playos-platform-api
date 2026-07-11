// PlayOS Platform API — umbrella header.
//
// The PlayOS Platform API is the engine-agnostic contract that applications
// call. It must not depend on any game engine (see playos-spec, principle
// "Engine Agnostic"). Raylib, SDL, Godot, or a custom engine may sit above it.
#pragma once

#include "playos/audio.h"
#include "playos/battery.h"
#include "playos/bluetooth.h"
#include "playos/capabilities.h"
#include "playos/display.h"
#include "playos/input.h"
#include "playos/lifecycle.h"
#include "playos/network.h"
#include "playos/storage.h"
#include "playos/touch.h"

#define PLAYOS_VERSION_MAJOR 0
#define PLAYOS_VERSION_MINOR 1
#define PLAYOS_VERSION_PATCH 0
