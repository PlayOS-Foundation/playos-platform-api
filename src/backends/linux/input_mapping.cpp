// PlayOS Platform API — Linux Input Mapping implementation.
//
// Lookup table: symbolic button names (RFC-0006 vocabulary) → evdev key codes.
// Add new entries here when adding a new device with vendor-specific buttons.
#include "input_mapping.h"

#include <linux/input-event-codes.h>
#include <unordered_map>

namespace PlayOS {
namespace Detail {
namespace {

using CodeList = std::vector<int>;

// Maps RFC-0006 symbolic button names to Linux evdev key codes.
// Multiple codes per name = shotgun approach for vendor variation.
// E.g. ROG Ally Armoury button might emit KEY_PROG1 or BTN_TRIGGER_HAPPY1
// depending on kernel version / firmware.
const std::unordered_map<std::string, CodeList> kInputMap = {
    // ── Standard buttons ─────────────────────────────────────────────────
    {"xbox_guide",             {BTN_MODE}},

    // ── ASUS ROG Ally ────────────────────────────────────────────────────
    {"asus_armoury",           {KEY_PROG1, BTN_TRIGGER_HAPPY1}},
    {"asus_command_center",    {KEY_PROG2, BTN_TRIGGER_HAPPY2}},

    // ── Steam Deck ───────────────────────────────────────────────────────
    {"steamdeck_qam",          {BTN_TRIGGER_HAPPY3}},
    {"steamdeck_steam",        {BTN_TRIGGER_HAPPY4}},

    // ── Lenovo Legion Go ─────────────────────────────────────────────────
    {"legion_quick",           {KEY_PROG3}},

    // ── Keyboard fallbacks (desktops, VMs) ───────────────────────────────
    {"keyboard_f1",            {KEY_F1}},
    {"keyboard_esc",           {KEY_ESC}},

    // ── Generic vendor button catch-all ──────────────────────────────────
    {"generic_guide",          {BTN_MODE, BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2,
                                BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4,
                                KEY_PROG1}},
};

} // namespace

std::vector<int> ResolveInputCode(const std::string& symbolicName) {
    auto it = kInputMap.find(symbolicName);
    if (it != kInputMap.end()) return it->second;
    return {};
}

} // namespace Detail
} // namespace PlayOS
