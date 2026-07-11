// Null network backend — no connectivity info (desktop SDK / test builds).
#include "backends/network_backend.h"
#include <vector>

namespace PlayOS {
namespace Detail {
namespace {

class NullNetworkBackend : public INetworkBackend {
public:
    Network::WiFiState GetWiFiState() override { return Network::WiFiState::Unknown; }
    const char* PrimaryIP() override { return ""; }
    std::vector<Network::WiFiNetwork> ScanNetworks() override { return {}; }
    Network::ConnectResult Connect(const std::string&, const std::string&) override {
        return Network::ConnectResult::Error;
    }
};

} // namespace

INetworkBackend* CreateNetworkBackend() {
    return new NullNetworkBackend();
}

} // namespace Detail
} // namespace PlayOS
