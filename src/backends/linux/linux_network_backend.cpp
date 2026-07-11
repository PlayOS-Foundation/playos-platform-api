// Linux network backend — uses getifaddrs for state/IP, nmcli for WiFi mgmt.
// All OS-specific network code belongs here, NOT in application code.
#include "backends/network_backend.h"
#include "playos/capabilities.h"

#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <sstream>
#include <string>
#include <vector>

namespace PlayOS {
namespace Detail {
namespace {

// Run a shell command and return its stdout output.
static std::string RunCmd(const char* cmd) {
    std::string out;
    FILE* f = popen(cmd, "r");
    if (!f) return out;
    char buf[256];
    while (fgets(buf, sizeof(buf), f))
        out += buf;
    pclose(f);
    return out;
}

class LinuxNetworkBackend : public INetworkBackend {
public:
    Network::WiFiState GetWiFiState() override {
        struct ifaddrs *ifaddr = nullptr;
        if (getifaddrs(&ifaddr) != 0) return Network::WiFiState::Unknown;

        Network::WiFiState best = Network::WiFiState::Absent;
        for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
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

    // nmcli -t -f SSID,SIGNAL,SECURITY,ACTIVE --escape no device wifi list
    // Output lines: SSID:SIGNAL:SECURITY:ACTIVE
    // SSID may contain colons — parse right-to-left.
    std::vector<Network::WiFiNetwork> ScanNetworks() override {
        std::vector<Network::WiFiNetwork> nets;
        // Rescan first (best-effort, may fail without privilege)
        RunCmd("nmcli device wifi rescan 2>/dev/null");
        std::string raw = RunCmd(
            "nmcli --escape no -t -f SSID,SIGNAL,SECURITY,ACTIVE "
            "device wifi list 2>/dev/null");

        std::istringstream ss(raw);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            // Split from the right: active(1), security(1), signal(1), rest=ssid
            auto rp1 = line.rfind(':');
            if (rp1 == std::string::npos) continue;
            std::string active = line.substr(rp1 + 1);
            line = line.substr(0, rp1);

            auto rp2 = line.rfind(':');
            if (rp2 == std::string::npos) continue;
            std::string security = line.substr(rp2 + 1);
            line = line.substr(0, rp2);

            auto rp3 = line.rfind(':');
            if (rp3 == std::string::npos) continue;
            std::string signal_s = line.substr(rp3 + 1);
            std::string ssid = line.substr(0, rp3);

            if (ssid.empty()) continue;  // hidden network

            Network::WiFiNetwork n;
            n.ssid     = ssid;
            n.signal   = std::stoi(signal_s.empty() ? "0" : signal_s);
            n.secured  = !security.empty();
            n.active   = (active == "yes");
            nets.push_back(std::move(n));
        }
        return nets;
    }

    Network::ConnectResult Connect(const std::string& ssid,
                                   const std::string& psk) override {
        // Build nmcli command safely — avoid shell injection via popen quoting.
        // We use single-quotes and escape any single-quote in ssid/psk.
        auto escape = [](const std::string& s) {
            std::string r;
            for (char c : s) {
                if (c == '\'') r += "'\\''";
                else r += c;
            }
            return r;
        };

        std::string cmd = "nmcli device wifi connect '"
                        + escape(ssid) + "'";
        if (!psk.empty())
            cmd += " password '" + escape(psk) + "'";
        cmd += " 2>&1";

        std::string out = RunCmd(cmd.c_str());

        if (out.find("successfully activated") != std::string::npos)
            return Network::ConnectResult::Success;
        if (out.find("Secrets were required") != std::string::npos ||
            out.find("password") != std::string::npos)
            return Network::ConnectResult::WrongPassword;
        if (out.find("Timeout") != std::string::npos ||
            out.find("timeout") != std::string::npos)
            return Network::ConnectResult::Timeout;
        return Network::ConnectResult::Error;
    }

private:
    std::string m_ip;
};

} // namespace

INetworkBackend* CreateNetworkBackend() {
    PlayOS::Capabilities::RegisterCapability(PlayOS::Capability::NetworkInfo);
    return new LinuxNetworkBackend();
}

} // namespace Detail
} // namespace PlayOS
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
