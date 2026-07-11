// PlayOS Platform API — Linux Input Mapping.
//
// Translates symbolic button names from a DeviceProfile into Linux evdev
// key codes. This is the platform-specific bridge between RFC-0006's
// portable input vocabulary and the kernel's input event codes.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace PlayOS {
namespace Detail {

/// Resolves a symbolic button name (e.g. "asus_armoury") to one or more
/// Linux evdev key codes (e.g. {KEY_PROG1, BTN_TRIGGER_HAPPY1}).
///
/// Returns an empty vector if the name is unknown. The caller should fall
/// back to the hardcoded default mapping in that case.
std::vector<int> ResolveInputCode(const std::string& symbolicName);

} // namespace Detail
} // namespace PlayOS
