#include "playos/input.h"

#include "backends/input_backend.h"

#include <array>
#include <memory>

namespace PlayOS {
namespace {

std::unique_ptr<Detail::IInputBackend> g_backend;

constexpr int kButtonCount = static_cast<int>(Button::Count);
std::array<bool, kButtonCount> g_prev{};
std::array<bool, kButtonCount> g_curr{};

Detail::IInputBackend* backend() {
    if (!g_backend) {
        g_backend.reset(Detail::CreateInputBackend());
    }
    return g_backend.get();
}

} // namespace

namespace Input {

void Update() {
    auto* b = backend();
    b->Update();
    for (int i = 0; i < kButtonCount; ++i) {
        g_prev[i] = g_curr[i];
        g_curr[i] = b->Down(static_cast<Button>(i));
    }
}

bool Down(Button button) {
    return g_curr[static_cast<int>(button)];
}

bool Pressed(Button button) {
    const int i = static_cast<int>(button);
    return g_curr[i] && !g_prev[i];
}

float GetAxis(Axis axis) {
    return backend()->GetAxis(axis);
}

bool ControllerConnected() {
    return backend()->Connected();
}

} // namespace Input
} // namespace PlayOS
