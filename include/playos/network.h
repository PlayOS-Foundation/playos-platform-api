// PlayOS Platform API — Network.
//
// Runtime network connectivity state. Gated on Capability::NetworkInfo.
// Applications MUST check Has(Capability::NetworkInfo) before calling these.
// See playos-spec: book/src/06-platform-api.
#pragma once

namespace PlayOS {
namespace Network {

// Connectivity state of the primary wireless interface.
enum class WiFiState {
    Unknown,      // No wireless hardware or state cannot be determined.
    Absent,       // No wireless hardware detected.
    Disconnected, // Hardware present, not associated to any network.
    Connecting,   // Associated but DHCP/IP assignment in progress.
    Connected,    // Associated and has a valid IPv4 address.
};

// Returns the current state of the primary wireless interface.
WiFiState GetWiFiState();

// Returns the primary non-loopback IPv4 address as a null-terminated string,
// or an empty string if no address is available.
// The returned pointer is valid until the next call to PrimaryIP().
const char* PrimaryIP();

} // namespace Network
} // namespace PlayOS
