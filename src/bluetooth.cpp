#include "playos/bluetooth.h"
#include "backends/bluetooth_backend.h"
#include <memory>

namespace PlayOS {
namespace {

std::unique_ptr<Detail::IBluetoothBackend> g_bt_backend;

Detail::IBluetoothBackend* bt_backend() {
    if (!g_bt_backend)
        g_bt_backend.reset(Detail::CreateBluetoothBackend());
    return g_bt_backend.get();
}

} // namespace

namespace Bluetooth {

bool IsPresent() { return bt_backend()->IsPresent(); }

} // namespace Bluetooth
} // namespace PlayOS
