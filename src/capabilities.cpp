#include "playos/capabilities.h"

#include <algorithm>

namespace PlayOS {
namespace Capabilities {
namespace {

// The set of capabilities this build reports. For the Windows SDK target the
// baseline required capabilities are provided. Runtime devices and richer
// backends will extend this registry (see playos-spec RFC-0003).
const std::vector<Capability>& registry() {
    static const std::vector<Capability> caps = {
        Capability::InputBasic,
        Capability::StorageLocal,
        Capability::DisplayInfo,
        Capability::LifecycleBasic,
    };
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
        case Capability::Battery:        return "power.battery";
        case Capability::Brightness:     return "display.brightness";
        case Capability::CloudSave:      return "cloud.saves";
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
