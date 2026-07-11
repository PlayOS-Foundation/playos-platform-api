// Internal Bluetooth backend interface.
#pragma once

namespace PlayOS {
namespace Detail {

class IBluetoothBackend {
public:
    virtual ~IBluetoothBackend() = default;
    virtual bool IsPresent() = 0;
};

IBluetoothBackend* CreateBluetoothBackend();

} // namespace Detail
} // namespace PlayOS
