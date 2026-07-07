// PlayOS Platform API — Input.
//
// Applications use logical PlayOS buttons and axes, not device-specific
// codes. Device backends map physical inputs to these logical names.
// See playos-spec: book/src/09-shell-and-ux/03-controller-first-navigation.
#pragma once

namespace PlayOS {

// Logical controller buttons. The integer values are contiguous starting at
// zero so the runtime can index button-state arrays; keep Home/QuickSettings
// last and do not reorder without updating input.cpp.
enum class Button {
    A = 0,
    B,
    X,
    Y,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    L1,
    R1,
    L2,
    R2,
    Start,
    Select,
    Home,
    QuickSettings,
    Count // sentinel — must remain last
};

// Logical analog axes. Values range [-1, 1] for sticks and [0, 1] for
// triggers.
enum class Axis {
    LeftX = 0,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,
    RightTrigger,
};

namespace Input {

// Polls the input backend and updates button edge state. Call once per frame
// (Lifecycle::Update calls this for you).
void Update();

// True while a button is held.
bool Down(Button button);

// True only on the frame a button transitions from up to down.
bool Pressed(Button button);

// Current value of an analog axis.
float GetAxis(Axis axis);

// True when a controller is physically connected and providing input.
bool ControllerConnected();

} // namespace Input
} // namespace PlayOS
