// Raylib display backend — wraps Raylib's monitor API behind
// IDisplayBackend. Works on Windows and Linux.
//
// This is the recommended backend for the vertical-slice PoC.

#include "backends/display_backend.h"

#include "raylib.h"

namespace PlayOS {
namespace Detail {
namespace {

class RaylibDisplayBackend : public IDisplayBackend {
public:
    DisplayInfo Query() override {
        DisplayInfo info{};
        // Get the current monitor's properties. On desktop this is the
        // monitor the window is on; on fullscreen it's the primary.
        int monitor = GetCurrentMonitor();
        info.width       = GetMonitorWidth(monitor);
        info.height      = GetMonitorHeight(monitor);
        info.refreshRate = GetMonitorRefreshRate(monitor);
        // Desktop SDK targets have no screen obstructions.
        info.safeAreaLeft   = 0;
        info.safeAreaTop    = 0;
        info.safeAreaRight  = 0;
        info.safeAreaBottom = 0;
        return info;
    }
};

} // namespace

IDisplayBackend* CreateDisplayBackend() {
    return new RaylibDisplayBackend();
}

} // namespace Detail
} // namespace PlayOS
