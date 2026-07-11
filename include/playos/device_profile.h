// PlayOS Platform API — Device Profile.
//
// Loads and validates a device profile TOML file per RFC-0006.
// Profiles live at /etc/playos/device-profiles/<id>.toml on runtime devices
// and in playos-reference-devices/<device>/device-profile.toml in the source tree.
//
// This is a cross-platform class. Platform-specific details (e.g. evdev key
// codes for input mappings) live in the backend's InputMapping.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace PlayOS {

/// Parsed device profile (RFC-0006).
class DeviceProfile {
public:
    // ── Identity ──────────────────────────────────────────────────────
    struct DeviceInfo {
        std::string id;
        std::string name;
        std::string targetType;   // "runtime-device" or "sdk-target"
    };

    // ── Input mapping ─────────────────────────────────────────────────
    struct InputMapping {
        std::string homeButton;           // symbolic name, e.g. "asus_armoury"
        std::string quickSettingsButton;  // symbolic name, e.g. "asus_command_center"
        std::string builtInControls;      // device identifier, e.g. "gamepad0"
        std::string touchscreen;          // device identifier, e.g. "touch0"
    };

    // ── Display ───────────────────────────────────────────────────────
    struct DisplayInfo {
        int defaultWidth = 1280;
        int defaultHeight = 720;
        int refreshRate = 60;
        struct Insets { int top = 0, bottom = 0, left = 0, right = 0; };
        std::optional<Insets> safeAreaInsets;
    };

    // ── Construction ──────────────────────────────────────────────────

    /// Load and parse a profile from a TOML file path.
    /// Returns std::nullopt on parse error or missing required sections.
    static std::optional<DeviceProfile> Load(const std::string& path);

    /// Empty profile with safe defaults (used when no profile file exists).
    static DeviceProfile Default();

    // ── Accessors ─────────────────────────────────────────────────────

    const DeviceInfo&   device()       const { return m_device; }
    const InputMapping& input()        const { return m_input; }
    const DisplayInfo&  display()      const { return m_display; }
    bool                hasCapability(const std::string& id) const;
    const std::string&  powerPath(const std::string& key) const;

private:
    DeviceInfo   m_device;
    InputMapping m_input;
    DisplayInfo  m_display;
    std::unordered_map<std::string, bool> m_capabilities;
    std::unordered_map<std::string, std::string> m_powerPaths;
    // m_vendor extensions parsed but accessible via raw TOML if needed
};

} // namespace PlayOS
