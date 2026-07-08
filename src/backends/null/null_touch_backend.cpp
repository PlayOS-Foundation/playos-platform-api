// Null touch backend — reports no touch points. Used on targets without
// touch hardware or as a placeholder.

#include "backends/touch_backend.h"

namespace PlayOS {
namespace Detail {
namespace {

class NullTouchBackend : public ITouchBackend {
public:
    int PointCount() override { return 0; }
    TouchPoint GetPoint(int) override { return {}; }
    bool Available() const override { return false; }
};

} // namespace

ITouchBackend* CreateTouchBackend() {
    return new NullTouchBackend();
}

} // namespace Detail
} // namespace PlayOS
