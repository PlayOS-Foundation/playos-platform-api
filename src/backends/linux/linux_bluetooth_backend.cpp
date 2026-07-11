// Linux Bluetooth backend — checks /sys/class/bluetooth/ for host adapters.
#include "backends/bluetooth_backend.h"
#include "playos/capabilities.h"
#include <dirent.h>

namespace PlayOS {
namespace Detail {
namespace {

class LinuxBluetoothBackend : public IBluetoothBackend {
public:
    bool IsPresent() override {
        if (m_searched) return m_present;
        m_searched = true;
        DIR *d = opendir("/sys/class/bluetooth");
        if (!d) return false;
        struct dirent *ent;
        while ((ent = readdir(d)) != nullptr) {
            if (ent->d_name[0] != '.') { m_present = true; break; }
        }
        closedir(d);
        return m_present;
    }

private:
    bool m_searched = false;
    bool m_present  = false;
};

} // namespace

IBluetoothBackend* CreateBluetoothBackend() {
    // Self-register only if adapter is actually present.
    auto *b = new LinuxBluetoothBackend();
    if (b->IsPresent())
        PlayOS::Capabilities::RegisterCapability(PlayOS::Capability::BluetoothPresent);
    return b;
}

} // namespace Detail
} // namespace PlayOS
