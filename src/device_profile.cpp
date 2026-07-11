// PlayOS Platform API — Device Profile implementation.
#include "playos/device_profile.h"

#include <toml++/toml.hpp>

#include <fstream>
#include <sstream>

namespace PlayOS {

// ── Default profile (safe fallback) ─────────────────────────────────────────

DeviceProfile DeviceProfile::Default() {
    DeviceProfile p;
    p.m_device.id = "generic";
    p.m_device.name = "Generic Device";
    p.m_device.targetType = "runtime-device";
    p.m_input.homeButton = "xbox_guide";
    p.m_display.defaultWidth = 1280;
    p.m_display.defaultHeight = 720;
    p.m_display.refreshRate = 60;
    return p;
}

// ── TOML loader ─────────────────────────────────────────────────────────────

std::optional<DeviceProfile> DeviceProfile::Load(const std::string& path) {
    // Read file into string for better error messages
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;

    std::stringstream buf;
    buf << file.rdbuf();
    std::string content = buf.str();
    file.close();

    toml::table root;
    try {
        root = toml::parse(content);
    } catch (const toml::parse_error&) {
        return std::nullopt;
    }

    // ── [device] (required) ─────────────────────────────────────────────
    auto* dev = root["device"].as_table();
    if (!dev) return std::nullopt;

    DeviceProfile p;

    p.m_device.id         = dev->get("id")->value_or("");
    p.m_device.name       = dev->get("name")->value_or("");
    p.m_device.targetType = dev->get("targetType")->value_or("runtime-device");
    if (p.m_device.id.empty() || p.m_device.name.empty()) return std::nullopt;

    // ── [input] (optional — runtime devices only) ──────────────────────
    if (auto* input = root["input"].as_table()) {
        p.m_input.homeButton          = input->get("home_button")->value_or("");
        p.m_input.quickSettingsButton = input->get("quick_settings_button")->value_or("");
        p.m_input.builtInControls     = input->get("built_in_controls")->value_or("");
        p.m_input.touchscreen         = input->get("touchscreen")->value_or("");
    }

    // ── [display] (required) ───────────────────────────────────────────
    if (auto* disp = root["display"].as_table()) {
        p.m_display.defaultWidth  = disp->get("default_width")->value_or(1280);
        p.m_display.defaultHeight = disp->get("default_height")->value_or(720);
        p.m_display.refreshRate   = disp->get("refresh_rate")->value_or(60);

        if (auto* insets = disp->get("safe_area_insets")->as_table()) {
            DisplayInfo::Insets i;
            i.top    = insets->get("top")->value_or(0);
            i.bottom = insets->get("bottom")->value_or(0);
            i.left   = insets->get("left")->value_or(0);
            i.right  = insets->get("right")->value_or(0);
            p.m_display.safeAreaInsets = i;
        }
    }

    // ── [capabilities] (required) ──────────────────────────────────────
    if (auto* caps = root["capabilities"].as_table()) {
        for (const auto& [key, val] : *caps) {
            if (auto b = val.as_boolean())
                p.m_capabilities[std::string(key)] = b->get();
        }
    }

    // ── [power] (optional) ─────────────────────────────────────────────
    if (auto* power = root["power"].as_table()) {
        for (const auto& [key, val] : *power) {
            if (auto s = val.as_string())
                p.m_powerPaths[std::string(key)] = std::string(s->get());
        }
    }

    return p;
}

// ── Capability check ────────────────────────────────────────────────────────

bool DeviceProfile::hasCapability(const std::string& id) const {
    auto it = m_capabilities.find(id);
    return it != m_capabilities.end() && it->second;
}

// ── Power path lookup ───────────────────────────────────────────────────────

const std::string& DeviceProfile::powerPath(const std::string& key) const {
    static const std::string kEmpty;
    auto it = m_powerPaths.find(key);
    return it != m_powerPaths.end() ? it->second : kEmpty;
}

} // namespace PlayOS
