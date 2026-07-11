// Linux network backend — uses getifaddrs to report WiFi state and primary IP.
// All OS-specific network code belongs here, NOT in application code.
#include "backends/network_backend.h"
#include "playos/capabilities.h"

#include <arpa/inet.h>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <string>

namespace PlayOS {
namespace Detail {
namespace {

class LinuxNetworkBackend : public INetworkBackend {
public:
    Network::WiFiState GetWiFiState() override {
        struct ifaddrs *ifaddr = nullptr;
        if (getifaddrs(&ifaddr) != 0) return Network::WiFiState::Unknown;

        Network::WiFiState best = Network::WiFiState::Absent;
        for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            // Wireless interfaces start with 'wl' (wlan0, wlp3s0, etc.)
            if (strncmp(ifa->ifa_name, "wl", 2) != 0) continue;
            if (!(ifa->ifa_flags & IFF_UP)) {
                if (best == Network::WiFiState::Absent)
                    best = Network::WiFiState::Disconnected;
                continue;
            }
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                best = Network::WiFiState::Connected;
                break;
            }
            // Interface UP but no IPv4 yet — DHCP in progress
            if (best != Network::WiFiState::Connected)
                best = Network::WiFiState::Connecting;
        }
        freeifaddrs(ifaddr);
        return best;
    }

    const char* PrimaryIP() override {
        m_ip.clear();
        struct ifaddrs *ifaddr = nullptr;
        if (getifaddrs(&ifaddr) != 0) return "";

        for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) continue;
            if (ifa->ifa_addr->sa_family != AF_INET) continue;
            if (ifa->ifa_flags & IFF_LOOPBACK) continue;
            if (!(ifa->ifa_flags & IFF_UP)) continue;

            char buf[INET_ADDRSTRLEN];
            void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr, buf, sizeof(buf));
            m_ip = buf;
            break;
        }
        freeifaddrs(ifaddr);
        return m_ip.c_str();
    }

private:
    std::string m_ip;
};

} // namespace

INetworkBackend* CreateNetworkBackend() {
    // Self-register: any build that links this backend advertises NetworkInfo.
    PlayOS::Capabilities::RegisterCapability(PlayOS::Capability::NetworkInfo);
    return new LinuxNetworkBackend();
}

} // namespace Detail
} // namespace PlayOS
