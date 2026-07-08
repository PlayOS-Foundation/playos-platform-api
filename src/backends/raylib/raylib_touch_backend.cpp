// Raylib touch backend — wraps Raylib's touch API behind ITouchBackend.
// Works on platforms where Raylib supports touch (Android, Web, desktop
// with touch-enabled monitor).

#include "backends/touch_backend.h"

#include "raylib.h"

namespace PlayOS {
namespace Detail {
namespace {

class RaylibTouchBackend : public ITouchBackend {
public:
    int PointCount() override {
        return GetTouchPointCount();
    }

    TouchPoint GetPoint(int index) override {
        TouchPoint pt{};
        ::Vector2 v = GetTouchPosition(index);
        pt.x  = static_cast<int>(v.x);
        pt.y  = static_cast<int>(v.y);
        pt.id = GetTouchPointId(index);
        return pt;
    }

    bool Available() const override {
        // Raylib exposes touch functions; availability is best-effort
        // on desktop (returns 0 count if no touch hardware). On Android
        // and Web this is always true.
        return GetTouchPointCount() > 0;
    }
};

} // namespace

ITouchBackend* CreateTouchBackend() {
    return new RaylibTouchBackend();
}

} // namespace Detail
} // namespace PlayOS
