// PlayOS Platform API — Battery.
//
// Battery level and charging state. Gated on the optional `battery` capability
// (Capability::Battery). Applications MUST check Has(Capability::Battery)
// before calling these functions.
// See playos-spec: book/src/06-platform-api/11-battery-api.md.
#pragma once

namespace PlayOS {
namespace Battery {

// Battery charge level in the range [0.0, 1.0].
// Returns -1.0 if the capability is not present or the level is unknown.
float Level();

// Returns true if the device is currently connected to external power
// (charging or fully charged). Returns false if on battery or unknown.
bool IsCharging();

} // namespace Battery
} // namespace PlayOS
