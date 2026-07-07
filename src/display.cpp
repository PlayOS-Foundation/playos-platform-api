#include "playos/display.h"

#include "backends/display_backend.h"

#include <memory>

namespace PlayOS {
namespace {

std::unique_ptr<Detail::IDisplayBackend> g_backend;

Detail::IDisplayBackend* backend() {
    if (!g_backend) {
        g_backend.reset(Detail::CreateDisplayBackend());
    }
    return g_backend.get();
}

} // namespace

namespace Display {

DisplayInfo Current() {
    return backend()->Query();
}

} // namespace Display
} // namespace PlayOS
