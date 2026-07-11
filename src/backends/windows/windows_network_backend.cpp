// Windows network backend — uses GetAdaptersAddresses() (IP Helper API).
#include "backends/network_backend.h"
#include "playos/capabilities.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace PlayOS {
namespace Detail {
namespace {

class WindowsNetworkBackend : public INetworkBackend {
public:
    Network::WiFiState GetWiFiState() override {
        auto adapters = GetAdapters();
        if (!adapters) return Network::WiFiState::Unknown;

        Network::WiFiState best = Network::WiFiState::Absent;
        for (auto *a = adapters.get(); a; a = a->Next) {
            if (a->IfType != IF_TYPE_IEEE80211) continue;
            if (a->OperStatus == IfOperStatusUp) {
                // Check if it has a unicast IP
                if (a->FirstUnicastAddress) {
                    best = Network::WiFiState::Connected;
                    break;
                }
                if (best != Network::WiFiState::Connected)
                    best = Network::WiFiState::Connecting;
            } else {
                if (best == Network::WiFiState::Absent)
                    best = Network::WiFiState::Disconnected;
            }
        }
        return best;
    }

    const char* PrimaryIP() override {
        m_ip.clear();
        auto adapters = GetAdapters();
        if (!adapters) return "";

        for (auto *a = adapters.get(); a; a = a->Next) {
            if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            if (a->OperStatus != IfOperStatusUp) continue;
            for (auto *u = a->FirstUnicastAddress; u; u = u->Next) {
                if (u->Address.lpSockaddr->sa_family != AF_INET) continue;
                char buf[INET_ADDRSTRLEN];
                auto *sa = (struct sockaddr_in*)u->Address.lpSockaddr;
                inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
                m_ip = buf;
                return m_ip.c_str();
            }
        }
        return "";
    }

private:
    std::string m_ip;

    struct AdapterDeleter {
        void operator()(IP_ADAPTER_ADDRESSES* p) { free(p); }
    };
    using AdapterPtr = std::unique_ptr<IP_ADAPTER_ADDRESSES, AdapterDeleter>;

    static AdapterPtr GetAdapters() {
        ULONG size = 15000;
        AdapterPtr buf;
        for (int tries = 0; tries < 3; ++tries) {
            buf.reset((IP_ADAPTER_ADDRESSES*)malloc(size));
            if (!buf) return nullptr;
            ULONG ret = GetAdaptersAddresses(AF_INET,
                GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                nullptr, buf.get(), &size);
            if (ret == NO_ERROR) return buf;
            if (ret != ERROR_BUFFER_OVERFLOW) return nullptr;
        }
        return nullptr;
    }
};

} // namespace

INetworkBackend* CreateNetworkBackend() {
    auto *b = new WindowsNetworkBackend();
    // NetworkInfo capability always present on Windows (has a network stack)
    PlayOS::Capabilities::RegisterCapability(PlayOS::Capability::NetworkInfo);
    return b;
}

} // namespace Detail
} // namespace PlayOS
