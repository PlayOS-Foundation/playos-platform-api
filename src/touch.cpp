#include "playos/touch.h"

#include "backends/touch_backend.h"

#include <memory>

namespace PlayOS {
namespace {

std::unique_ptr<Detail::ITouchBackend> g_backend;

Detail::ITouchBackend* backend() {
    if (!g_backend) {
        g_backend.reset(Detail::CreateTouchBackend());
    }
    return g_backend.get();
}

} // namespace

namespace Touch {

int PointCount() {
    return backend()->PointCount();
}

TouchPoint GetPoint(int index) {
    return backend()->GetPoint(index);
}

} // namespace Touch
} // namespace PlayOS
