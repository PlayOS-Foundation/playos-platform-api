// Null Bluetooth backend — no Bluetooth (desktop SDK / test builds).
#include "backends/bluetooth_backend.h"

namespace PlayOS {
namespace Detail {
namespace {

class NullBluetoothBackend : public IBluetoothBackend {
public:
    bool IsPresent() override { return false; }
};

} // namespace

IBluetoothBackend* CreateBluetoothBackend() {
    return new NullBluetoothBackend();
}

} // namespace Detail
} // namespace PlayOS
