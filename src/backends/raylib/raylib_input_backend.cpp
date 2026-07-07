// Raylib input backend — wraps Raylib's built-in gamepad API behind
// IInputBackend. Works on Windows and Linux; supports any controller GLFW's
// gamepad mapping database knows (DualSense, Switch Pro, Xbox, etc.).
//
// This is the recommended backend for the vertical-slice PoC. On a runtime
// device the compositor routes input through the Wayland seat; this backend
// is used during development on SDK targets.

#include "backends/input_backend.h"

#include "raylib.h"

namespace PlayOS {
namespace Detail {
namespace {

// GLFW's gamepad button -> PlayOS logical button
Button MapRaylibButton(int rb) {
    switch (rb) {
        case GAMEPAD_BUTTON_RIGHT_FACE_DOWN:  return Button::A;       // Cross / A
        case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: return Button::B;       // Circle / B
        case GAMEPAD_BUTTON_RIGHT_FACE_LEFT:  return Button::X;       // Square / X
        case GAMEPAD_BUTTON_RIGHT_FACE_UP:    return Button::Y;       // Triangle / Y
        case GAMEPAD_BUTTON_MIDDLE_LEFT:      return Button::Select;  // Share / Back
        case GAMEPAD_BUTTON_MIDDLE_RIGHT:     return Button::Start;   // Options / Start
        case GAMEPAD_BUTTON_LEFT_TRIGGER_1:   return Button::L1;
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_1:  return Button::R1;
        case GAMEPAD_BUTTON_LEFT_TRIGGER_2:   return Button::L2;
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_2:  return Button::R2;
        case GAMEPAD_BUTTON_LEFT_FACE_UP:     return Button::DPadUp;
        case GAMEPAD_BUTTON_LEFT_FACE_DOWN:   return Button::DPadDown;
        case GAMEPAD_BUTTON_LEFT_FACE_LEFT:   return Button::DPadLeft;
        case GAMEPAD_BUTTON_LEFT_FACE_RIGHT:  return Button::DPadRight;
        case GAMEPAD_BUTTON_MIDDLE:          return Button::Home;     // PS / Xbox guide
        default: return Button::Count;
    }
}

class RaylibInputBackend : public IInputBackend {
public:
    void Update() override {
        // Nothing to poll; Raylib updates gamepad state internally each
        // frame before we query it.
    }

    bool Down(Button button) override {
        if (!connected()) return false;
        for (int slot = 0; slot < 4; ++slot) {
            if (!IsGamepadAvailable(slot)) continue;
            for (int rb = 0; rb < GAMEPAD_BUTTON_MIDDLE + 1; ++rb) {
                if (MapRaylibButton(rb) == button &&
                    IsGamepadButtonDown(slot, rb)) {
                    return true;
                }
            }
        }
        return false;
    }

    float GetAxis(Axis axis) override {
        if (!connected()) return 0.0f;
        for (int slot = 0; slot < 4; ++slot) {
            if (!IsGamepadAvailable(slot)) continue;
            switch (axis) {
                case Axis::LeftX:
                    return GetGamepadAxisMovement(slot, GAMEPAD_AXIS_LEFT_X);
                case Axis::LeftY:
                    return GetGamepadAxisMovement(slot, GAMEPAD_AXIS_LEFT_Y);
                case Axis::RightX:
                    return GetGamepadAxisMovement(slot, GAMEPAD_AXIS_RIGHT_X);
                case Axis::RightY:
                    return GetGamepadAxisMovement(slot, GAMEPAD_AXIS_RIGHT_Y);
                case Axis::LeftTrigger:
                    return GetGamepadAxisMovement(slot, GAMEPAD_AXIS_LEFT_TRIGGER);
                case Axis::RightTrigger:
                    return GetGamepadAxisMovement(slot, GAMEPAD_AXIS_RIGHT_TRIGGER);
            }
        }
        return 0.0f;
    }

    bool Connected() const override {
        for (int slot = 0; slot < 4; ++slot) {
            if (IsGamepadAvailable(slot)) return true;
        }
        return false;
    }

private:
    bool connected() const { return Connected(); }
};

} // namespace

IInputBackend* CreateInputBackend() {
    return new RaylibInputBackend();
}

} // namespace Detail
} // namespace PlayOS
