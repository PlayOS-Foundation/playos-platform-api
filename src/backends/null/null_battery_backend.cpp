// Null battery backend — reports no battery (desktop / no power supply).
#include "backends/battery_backend.h"

namespace PlayOS {
namespace Detail {
namespace {

class NullBatteryBackend : public IBatteryBackend {
public:
    float Level() override { return -1.0f; }
    bool IsCharging() override { return false; }
};

} // namespace

IBatteryBackend* CreateBatteryBackend() {
    return new NullBatteryBackend();
}

} // namespace Detail
} // namespace PlayOS
