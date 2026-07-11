#include "playos/network.h"
#include "backends/network_backend.h"
#include <memory>

namespace PlayOS {
namespace {

std::unique_ptr<Detail::INetworkBackend> g_network_backend;

Detail::INetworkBackend* network_backend() {
    if (!g_network_backend)
        g_network_backend.reset(Detail::CreateNetworkBackend());
    return g_network_backend.get();
}

} // namespace

namespace Network {

WiFiState GetWiFiState() { return network_backend()->GetWiFiState(); }
const char* PrimaryIP()  { return network_backend()->PrimaryIP(); }

std::vector<WiFiNetwork> ScanNetworks() {
    return network_backend()->ScanNetworks();
}

ConnectResult Connect(const std::string& ssid, const std::string& psk) {
    return network_backend()->Connect(ssid, psk);
}

} // namespace Network
} // namespace PlayOS
