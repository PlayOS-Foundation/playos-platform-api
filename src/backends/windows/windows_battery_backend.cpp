// Windows battery backend — uses GetSystemPowerStatus() Win32 API.
#include "backends/battery_backend.h"
#include "playos/capabilities.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace PlayOS {
namespace Detail {
namespace {

class WindowsBatteryBackend : public IBatteryBackend {
public:
    float Level() override {
        SYSTEM_POWER_STATUS s;
        if (!GetSystemPowerStatus(&s)) return -1.0f;
        // 255 = no battery / unknown
        if (s.BatteryLifePercent == 255) return -1.0f;
        return (float)s.BatteryLifePercent / 100.0f;
    }

    bool IsCharging() override {
        SYSTEM_POWER_STATUS s;
        if (!GetSystemPowerStatus(&s)) return false;
        // ACLineStatus: 0=offline, 1=online(charging), 255=unknown
        return s.ACLineStatus == 1;
    }
};

} // namespace

IBatteryBackend* CreateBatteryBackend() {
    SYSTEM_POWER_STATUS s;
    auto *b = new WindowsBatteryBackend();
    // Self-register only if a real battery is present
    if (GetSystemPowerStatus(&s) && s.BatteryLifePercent != 255)
        PlayOS::Capabilities::RegisterCapability(PlayOS::Capability::Battery);
    return b;
}

} // namespace Detail
} // namespace PlayOS
