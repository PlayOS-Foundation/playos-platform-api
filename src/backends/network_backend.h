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
    virtual std::vector<Network::WiFiNetwork> ScanNetworks() = 0;
    virtual Network::ConnectResult Connect(const std::string& ssid,
                                           const std::string& psk) = 0;
};

INetworkBackend* CreateNetworkBackend();

} // namespace Detail
} // namespace PlayOS
