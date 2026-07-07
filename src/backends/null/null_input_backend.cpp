// Null input backend for platforms without a real backend yet. Reports no
// input. Keeps the Platform API buildable everywhere until a native backend
// (Linux/evdev, etc.) is added.
#include "backends/input_backend.h"

namespace PlayOS {
namespace Detail {
namespace {

class NullInputBackend : public IInputBackend {
public:
    void Update() override {}
    bool Down(Button) override { return false; }
    float GetAxis(Axis) override { return 0.0f; }
    bool Connected() const override { return false; }
};

} // namespace

IInputBackend* CreateInputBackend() {
    return new NullInputBackend();
}

} // namespace Detail
} // namespace PlayOS
