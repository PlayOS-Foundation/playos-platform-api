// Internal network backend interface.
#pragma once
#include "playos/network.h"

namespace PlayOS {
namespace Detail {

class INetworkBackend {
public:
    virtual ~INetworkBackend() = default;
    virtual Network::WiFiState GetWiFiState() = 0;
    virtual const char* PrimaryIP() = 0;
};

INetworkBackend* CreateNetworkBackend();

} // namespace Detail
} // namespace PlayOS
