// PlayOS Platform API — umbrella header.
//
// The PlayOS Platform API is the engine-agnostic contract that applications
// call. It must not depend on any game engine (see playos-spec, principle
// "Engine Agnostic"). Raylib, SDL, Godot, or a custom engine may sit above it.
#pragma once

#include "playos/capabilities.h"
#include "playos/input.h"
#include "playos/lifecycle.h"

#define PLAYOS_VERSION_MAJOR 0
#define PLAYOS_VERSION_MINOR 1
#define PLAYOS_VERSION_PATCH 0
