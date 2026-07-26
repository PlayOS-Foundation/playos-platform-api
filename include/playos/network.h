// PlayOS Platform API — Network.
//
// Runtime network connectivity state and WiFi management.
// Gated on Capability::NetworkInfo.
// Applications MUST check Has(Capability::NetworkInfo) before calling these.
// See playos-spec: book/src/06-platform-api.
#pragma once

#include <string>
#include <vector>

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

// Result of a Connect() call.
enum class ConnectResult {
    Success,       // Connected and IP obtained.
    WrongPassword, // Authentication rejected.
    Timeout,       // Did not connect within the timeout.
    Error,         // Other error (no hardware, SSID not found, etc.).
};

// A WiFi network visible during a scan.
struct WiFiNetwork {
    std::string ssid;     // Network name.
    int         signal;   // Signal strength 0–100.
    bool        secured;  // Requires a password.
    bool        active;   // Currently connected to this network.
};

// Returns the current state of the primary wireless interface.
WiFiState GetWiFiState();

// Returns the primary non-loopback IPv4 address as a null-terminated string,
// or an empty string if no address is available.
// The returned pointer is valid until the next call to PrimaryIP().
const char* PrimaryIP();

// ── Blocking (legacy — will block for 2–15 seconds) ───────────────────────

// Scans for nearby WiFi networks. Blocks for a few seconds.
// Returns an empty vector when no wireless hardware is present.
std::vector<WiFiNetwork> ScanNetworks();

// Connect to an SSID with an optional PSK (pass empty string for open networks).
// Blocks until connected, authentication failure, or timeout (~15 s).
ConnectResult Connect(const std::string& ssid, const std::string& psk);

// ── Non-blocking (preferred — never blocks the main thread) ───────────────

// Start a background WiFi scan. No-op if a scan is already running.
// Call PollScan() each frame until it returns true.
void StartScan();

// Returns true when the scan completes, populating `out` with results.
// After returning true the results are consumed; subsequent calls return false
// until a new StartScan() is issued.
bool PollScan(std::vector<WiFiNetwork>& out);

// Start a background WiFi connect. No-op if an operation is already running.
// Call PollConnect() each frame until it returns true.
void StartConnect(const std::string& ssid, const std::string& psk);

// Returns true when the connect completes, populating `out` with the result.
// After returning true the result is consumed; subsequent calls return false
// until a new StartConnect() is issued.
bool PollConnect(ConnectResult& out);

} // namespace Network
} // namespace PlayOS
