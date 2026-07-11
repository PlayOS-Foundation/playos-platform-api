#include "playos/capabilities.h"
#include "playos/battery.h"

#include <algorithm>
#include <vector>

namespace PlayOS {
namespace Capabilities {
namespace {

// Extra capabilities registered by backend libraries at static-init time.
// Network and Bluetooth backends call RegisterCapability() from their
// translation unit to advertise themselves — no direct dependency from the
// core library onto the backend headers.
static std::vector<Capability>& extraCaps() {
    static std::vector<Capability> v;
    return v;
}

} // namespace

// Called by backend translation units (network, bluetooth, etc.) to
// declare a runtime capability. Must be called before Capabilities::Has().
void RegisterCapability(Capability cap) {
    auto& v = extraCaps();
    if (std::find(v.begin(), v.end(), cap) == v.end())
        v.push_back(cap);
}

namespace {

const std::vector<Capability>& registry() {
    static std::vector<Capability> caps = []() {
        std::vector<Capability> c = {
            Capability::InputBasic,
            Capability::StorageLocal,
            Capability::DisplayInfo,
            Capability::LifecycleBasic,
        };
        // Battery: present when the platform reports a valid charge level.
        if (Battery::Level() >= 0.0f)
            c.push_back(Capability::Battery);
        // Network and Bluetooth capabilities are registered by their backends.
        for (auto cap : extraCaps())
            c.push_back(cap);
        return c;
    }();
    return caps;
}

} // namespace

bool Has(Capability capability) {
    const auto& caps = registry();
    return std::find(caps.begin(), caps.end(), capability) != caps.end();
}

std::vector<Capability> List() {
    return registry();
}

const char* Id(Capability capability) {
    switch (capability) {
        case Capability::InputBasic:     return "input.basic";
        case Capability::StorageLocal:   return "storage.local";
        case Capability::DisplayInfo:    return "display.info";
        case Capability::LifecycleBasic: return "lifecycle.basic";
        case Capability::Touch:          return "input.touch";
        case Capability::Battery:          return "power.battery";
        case Capability::Brightness:        return "display.brightness";
        case Capability::NetworkInfo:       return "network.info";
        case Capability::BluetoothPresent:  return "bluetooth.present";
        case Capability::CloudSave:         return "cloud.saves";
        case Capability::Overlay:        return "system.overlay";
        case Capability::Marketplace:    return "system.marketplace";
        case Capability::Suspend:        return "system.suspend";
        case Capability::Achievements:   return "cloud.achievements";
        case Capability::Leaderboards:   return "cloud.leaderboards";
    }
    return "unknown";
}

} // namespace Capabilities
} // namespace PlayOS
