// Windows input backend using XInput (gamepad). Windows is the primary SDK
// target for early development; the ROG Ally's built-in controls appear as a
// standard XInput device.
#include "backends/input_backend.h"

#include <windows.h>
#include <Xinput.h>

#include <cmath>

namespace PlayOS {
namespace Detail {
namespace {

constexpr float kStickMax = 32767.0f;
constexpr float kTriggerMax = 255.0f;

class WindowsInputBackend : public IInputBackend {
public:
    WindowsInputBackend() { findController(); }

    void Update() override {
        if (slot_ < 0) { findController(); if (slot_ < 0) return; }
        ZeroMemory(&state_, sizeof(state_));
        connected_ = (XInputGetState(slot_, &state_) == ERROR_SUCCESS);
        if (!connected_) { slot_ = -1; } // controller disconnected
    }

    bool Down(Button button) override {
        if (!connected_) return false;
        const WORD b = state_.Gamepad.wButtons;
        switch (button) {
            case Button::A:            return (b & XINPUT_GAMEPAD_A) != 0;
            case Button::B:            return (b & XINPUT_GAMEPAD_B) != 0;
            case Button::X:            return (b & XINPUT_GAMEPAD_X) != 0;
            case Button::Y:            return (b & XINPUT_GAMEPAD_Y) != 0;
            case Button::DPadUp:       return (b & XINPUT_GAMEPAD_DPAD_UP) != 0;
            case Button::DPadDown:     return (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
            case Button::DPadLeft:     return (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
            case Button::DPadRight:    return (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
            case Button::L1:           return (b & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
            case Button::R1:           return (b & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
            case Button::L2:           return state_.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            case Button::R2:           return state_.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            case Button::Start:        return (b & XINPUT_GAMEPAD_START) != 0;
            case Button::Select:       return (b & XINPUT_GAMEPAD_BACK) != 0;
            // Home / QuickSettings have no standard XInput mapping; on real
            // PlayOS devices these come from vendor buttons via the device
            // profile. Left unmapped on the Windows SDK target.
            case Button::Home:         return false;
            case Button::QuickSettings:return false;
            default:                   return false;
        }
    }

    float GetAxis(Axis axis) override {
        if (!connected_) return 0.0f;
        const auto& g = state_.Gamepad;
        switch (axis) {
            case Axis::LeftX:        return g.sThumbLX / kStickMax;
            case Axis::LeftY:        return g.sThumbLY / kStickMax;
            case Axis::RightX:       return g.sThumbRX / kStickMax;
            case Axis::RightY:       return g.sThumbRY / kStickMax;
            case Axis::LeftTrigger:  return g.bLeftTrigger / kTriggerMax;
            case Axis::RightTrigger: return g.bRightTrigger / kTriggerMax;
            default:                 return 0.0f;
        }
    }

private:
    void findController() {
        for (int i = 0; i < 4; ++i) {
            XINPUT_STATE s{};
            if (XInputGetState(i, &s) == ERROR_SUCCESS) {
                slot_ = i;
                state_ = s;
                connected_ = true;
                return;
            }
        }
        slot_ = -1;
        connected_ = false;
    }

    XINPUT_STATE state_{};
    bool connected_ = false;
    int slot_ = -1;   // XInput player slot, -1 = not found
};

} // namespace

IInputBackend* CreateInputBackend() {
    return new WindowsInputBackend();
}

} // namespace Detail
} // namespace PlayOS
