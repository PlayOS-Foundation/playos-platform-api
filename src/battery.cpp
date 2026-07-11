#include "playos/battery.h"
#include "backends/battery_backend.h"
#include <memory>

namespace PlayOS {
namespace {

std::unique_ptr<Detail::IBatteryBackend> g_battery_backend;

Detail::IBatteryBackend* battery_backend() {
    if (!g_battery_backend) {
        g_battery_backend.reset(Detail::CreateBatteryBackend());
    }
    return g_battery_backend.get();
}

} // namespace

namespace Battery {

float Level() {
    return battery_backend()->Level();
}

bool IsCharging() {
    return battery_backend()->IsCharging();
}

} // namespace Battery
} // namespace PlayOS
