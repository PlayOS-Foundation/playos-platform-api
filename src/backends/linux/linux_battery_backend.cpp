// Linux battery backend — reads /sys/class/power_supply/ (sysfs).
// Works on any Linux device that exposes a standard power supply node:
// ROG Ally, laptops, handheld PCs, etc.

#include "backends/battery_backend.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <dirent.h>

namespace PlayOS {
namespace Detail {
namespace {

// Read a single trimmed line from a sysfs file. Returns "" on failure.
static std::string ReadSysfs(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return {};
    char buf[64] = {};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    // Trim trailing newline
    size_t len = strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
    return buf;
}

// Find the first power_supply node that is a Battery type.
// Returns the path prefix e.g. "/sys/class/power_supply/BAT0"
static std::string FindBatteryNode() {
    const char* base = "/sys/class/power_supply";
    DIR* dir = opendir(base);
    if (!dir) return {};
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        std::string path = std::string(base) + "/" + ent->d_name;
        std::string type = ReadSysfs((path + "/type").c_str());
        if (type == "Battery") {
            closedir(dir);
            return path;
        }
    }
    closedir(dir);
    return {};
}

class LinuxBatteryBackend : public IBatteryBackend {
public:
    float Level() override {
        const std::string& node = BatteryNode();
        if (node.empty()) return -1.0f;

        // Prefer energy_full + energy_now (µWh), fall back to capacity (%)
        std::string full = ReadSysfs((node + "/energy_full").c_str());
        std::string now  = ReadSysfs((node + "/energy_now").c_str());
        if (!full.empty() && !now.empty()) {
            long f = atol(full.c_str());
            long n = atol(now.c_str());
            if (f > 0) return (float)n / (float)f;
        }

        std::string cap = ReadSysfs((node + "/capacity").c_str());
        if (!cap.empty()) {
            int pct = atoi(cap.c_str());
            return (float)pct / 100.0f;
        }

        return -1.0f;
    }

    bool IsCharging() override {
        const std::string& node = BatteryNode();
        if (node.empty()) return false;
        std::string status = ReadSysfs((node + "/status").c_str());
        // Possible values: Charging, Discharging, Not charging, Full, Unknown
        return status == "Charging" || status == "Full";
    }

private:
    const std::string& BatteryNode() {
        if (!m_searched) {
            m_node = FindBatteryNode();
            m_searched = true;
        }
        return m_node;
    }

    std::string m_node;
    bool m_searched = false;
};

} // namespace

IBatteryBackend* CreateBatteryBackend() {
    return new LinuxBatteryBackend();
}

} // namespace Detail
} // namespace PlayOS
