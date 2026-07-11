// PlayOS Platform API — Bluetooth.
//
// Bluetooth hardware presence query. Gated on Capability::BluetoothPresent.
// Applications MUST check Has(Capability::BluetoothPresent) before calling.
// See playos-spec: book/src/06-platform-api.
#pragma once

namespace PlayOS {
namespace Bluetooth {

// Returns true if a Bluetooth host controller adapter is present.
bool IsPresent();

} // namespace Bluetooth
} // namespace PlayOS
