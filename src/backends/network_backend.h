// Internal network backend interface.
#pragma once
#include "playos/network.h"
#include <string>
#include <vector>

namespace PlayOS {
namespace Detail {

class INetworkBackend {
public:
    virtual ~INetworkBackend() = default;
    virtual Network::WiFiState GetWiFiState() = 0;
    virtual const char* PrimaryIP() = 0;

    // ── Blocking (legacy) ─────────────────────────────────────────────────
    virtual std::vector<Network::WiFiNetwork> ScanNetworks() = 0;
    virtual Network::ConnectResult Connect(const std::string& ssid,
                                           const std::string& psk) = 0;

    // ── Non-blocking (async) ──────────────────────────────────────────────
    // Start a background scan. No-op if a scan is already running.
    virtual void StartScan() = 0;
    // Returns true when the scan completes, populating `out`.
    virtual bool PollScan(std::vector<Network::WiFiNetwork>& out) = 0;

    // Start a background connect. No-op if a connect is already running.
    virtual void StartConnect(const std::string& ssid,
                              const std::string& psk) = 0;
    // Returns true when the connect completes, populating `out`.
    virtual bool PollConnect(Network::ConnectResult& out) = 0;
};

INetworkBackend* CreateNetworkBackend();

} // namespace Detail
} // namespace PlayOS
