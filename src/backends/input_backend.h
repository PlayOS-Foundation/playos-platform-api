// Internal input backend interface. Each platform implements one backend;
// the public PlayOS::Input API (input.cpp) delegates to it and adds
// edge-detection (Pressed) on top of the backend's Down() state.
#pragma once

#include "playos/input.h"

namespace PlayOS {
namespace Detail {

class IInputBackend {
public:
    virtual ~IInputBackend() = default;

    // Poll hardware / OS input state.
    virtual void Update() = 0;

    // Is the logical button currently held?
    virtual bool Down(Button button) = 0;

    // Current value of the logical axis.
    virtual float GetAxis(Axis axis) = 0;

    // True when a physical controller is connected.
    virtual bool Connected() const = 0;
};

// Creates the platform-selected input backend. Implemented by exactly one
// backend translation unit (selected in CMakeLists.txt).
IInputBackend* CreateInputBackend();

} // namespace Detail
} // namespace PlayOS
