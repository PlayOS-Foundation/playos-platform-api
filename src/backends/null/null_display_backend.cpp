// Null display backend — returns safe defaults when no real display
// information is available. Used as a placeholder or on headless targets.

#include "backends/display_backend.h"

namespace PlayOS {
namespace Detail {
namespace {

class NullDisplayBackend : public IDisplayBackend {
public:
    DisplayInfo Query() override {
        DisplayInfo info{};
        info.width       = 1280;
        info.height      = 720;
        info.refreshRate = 60;
        info.safeAreaLeft   = 0;
        info.safeAreaTop    = 0;
        info.safeAreaRight  = 0;
        info.safeAreaBottom = 0;
        return info;
    }
};

} // namespace

IDisplayBackend* CreateDisplayBackend() {
    return new NullDisplayBackend();
}

} // namespace Detail
} // namespace PlayOS
