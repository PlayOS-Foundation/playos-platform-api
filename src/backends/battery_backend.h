// Internal battery backend interface. Each platform implements one backend;
// the public PlayOS::Battery API (battery.cpp) delegates to it.
#pragma once

namespace PlayOS {
namespace Detail {

class IBatteryBackend {
public:
    virtual ~IBatteryBackend() = default;

    // Returns battery level in [0.0, 1.0], or -1.0 if unavailable.
    virtual float Level() = 0;

    // Returns true if connected to external power.
    virtual bool IsCharging() = 0;
};

// Creates the platform-selected battery backend.
IBatteryBackend* CreateBatteryBackend();

} // namespace Detail
} // namespace PlayOS
