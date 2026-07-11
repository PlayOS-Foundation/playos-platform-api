// Null network backend — no connectivity info (desktop SDK / test builds).
#include "backends/network_backend.h"

namespace PlayOS {
namespace Detail {
namespace {

class NullNetworkBackend : public INetworkBackend {
public:
    Network::WiFiState GetWiFiState() override { return Network::WiFiState::Unknown; }
    const char* PrimaryIP() override { return ""; }
};

} // namespace

INetworkBackend* CreateNetworkBackend() {
    return new NullNetworkBackend();
}

} // namespace Detail
} // namespace PlayOS
