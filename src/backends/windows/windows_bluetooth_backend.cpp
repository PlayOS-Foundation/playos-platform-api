// Windows Bluetooth backend — uses BluetoothFindFirstRadio() Win32 API.
#include "backends/bluetooth_backend.h"
#include "playos/capabilities.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bluetoothapis.h>

#pragma comment(lib, "bthprops.lib")

namespace PlayOS {
namespace Detail {
namespace {

class WindowsBluetoothBackend : public IBluetoothBackend {
public:
    bool IsPresent() override {
        if (m_searched) return m_present;
        m_searched = true;
        BLUETOOTH_FIND_RADIO_PARAMS params = { sizeof(params) };
        HANDLE radio = nullptr;
        HBLUETOOTH_RADIO_FIND find = BluetoothFindFirstRadio(&params, &radio);
        if (find) {
            m_present = true;
            if (radio) CloseHandle(radio);
            BluetoothFindRadioClose(find);
        }
        return m_present;
    }

private:
    bool m_searched = false;
    bool m_present  = false;
};

} // namespace

IBluetoothBackend* CreateBluetoothBackend() {
    auto *b = new WindowsBluetoothBackend();
    if (b->IsPresent())
        PlayOS::Capabilities::RegisterCapability(PlayOS::Capability::BluetoothPresent);
    return b;
}

} // namespace Detail
} // namespace PlayOS
