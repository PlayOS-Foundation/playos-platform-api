// Linux display backend — reads display dimensions from the Wayland
// environment or falls back to the Raylib monitor query.
//
// On a PlayOS runtime device the shell runs as a Wayland client of the
// PlayOS compositor. The compositor configures the XDG toplevel to the
// full output size via wlr_xdg_toplevel_set_size(). After the first
// frame, Raylib's GetScreenWidth/Height returns the compositor-assigned
// size, which is the authoritative display size.
//
// Safe area insets for the ROG Ally are 0 — the screen has no notch,
// rounded corners do not cut into the logical pixel grid, and the bezel
// is outside the panel. Other runtime devices with physical obstructions
// should provide a device-specific backend or a device-profile override.

#include "backends/display_backend.h"
#include "raylib.h"

#include <cstdlib>

namespace PlayOS {
namespace Detail {
namespace {

// Read the PLAYOS_SAFE_AREA_{LEFT,TOP,RIGHT,BOTTOM} environment variables
// that the compositor or device-profile service may inject at startup.
// Returns 0 if not set.
static int SafeAreaEnv(const char* var) {
    const char* v = getenv(var);
    return v ? atoi(v) : 0;
}

class LinuxDisplayBackend : public IDisplayBackend {
public:
    DisplayInfo Query() override {
        DisplayInfo info{};

        // Primary source: screen dimensions as reported by Raylib after the
        // compositor has configured the window to the output size.
        int w = GetScreenWidth();
        int h = GetScreenHeight();

        // Raylib returns 0x0 before the window is initialised or when the
        // Wayland compositor has not yet sent a configure event.  Fall back
        // to the monitor physical size from GLFW/Wayland in that case.
        if (w <= 0 || h <= 0) {
            int monitor = GetCurrentMonitor();
            w = GetMonitorWidth(monitor);
            h = GetMonitorHeight(monitor);
        }

        info.width       = w;
        info.height      = h;
        info.refreshRate = GetMonitorRefreshRate(GetCurrentMonitor());

        // Safe area insets: read from environment (set by device profile
        // service or compositor) or default to 0 for devices without
        // physical screen obstructions (ROG Ally, desktop monitors).
        info.safeAreaLeft   = SafeAreaEnv("PLAYOS_SAFE_AREA_LEFT");
        info.safeAreaTop    = SafeAreaEnv("PLAYOS_SAFE_AREA_TOP");
        info.safeAreaRight  = SafeAreaEnv("PLAYOS_SAFE_AREA_RIGHT");
        info.safeAreaBottom = SafeAreaEnv("PLAYOS_SAFE_AREA_BOTTOM");

        return info;
    }
};

} // namespace

IDisplayBackend* CreateDisplayBackend() {
    return new LinuxDisplayBackend();
}

} // namespace Detail
} // namespace PlayOS
