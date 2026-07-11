// PlayOS Platform API — Capabilities.
//
// Applications discover what a device can do by querying capabilities at
// runtime, rather than checking the platform or operating system.
// See playos-spec: RFC-0003 and book/src/05-capability-model.
#pragma once

#include <vector>

namespace PlayOS {

// A named, discoverable device feature. Identifiers are namespaced strings
// (see Capabilities::Id), e.g. "input.basic", "cloud.saves".
enum class Capability {
    // Required — every conformant target provides these.
    InputBasic,
    StorageLocal,
    DisplayInfo,
    LifecycleBasic,

    // Optional — may or may not be present; query before use.
    Touch,
    Battery,
    Brightness,
    NetworkInfo,      // PlayOS::Network::GetWiFiState / PrimaryIP
    BluetoothPresent, // PlayOS::Bluetooth::IsPresent
    CloudSave,
    Overlay,
    Marketplace,
    Suspend,
    Achievements,
    Leaderboards,
};

namespace Capabilities {

// Returns whether a capability is available on this device right now.
bool Has(Capability capability);

// Enumerates all capabilities present on this device.
std::vector<Capability> List();

// Returns the stable namespaced identifier for a capability,
// e.g. Capability::InputBasic -> "input.basic".
const char* Id(Capability capability);

// Called by backend libraries to self-register a capability at startup.
// Application code should NOT call this directly.
void RegisterCapability(Capability capability);

} // namespace Capabilities
} // namespace PlayOS
