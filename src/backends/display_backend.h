// Internal display backend interface. Each platform/engine implements one
// backend; the public PlayOS::Display API (display.cpp) delegates to it.
#pragma once

#include "playos/display.h"

namespace PlayOS {
namespace Detail {

class IDisplayBackend {
public:
    virtual ~IDisplayBackend() = default;

    // Query current display properties.
    virtual DisplayInfo Query() = 0;
};

// Creates the platform-selected display backend. Implemented by exactly one
// backend translation unit (selected in CMakeLists.txt).
IDisplayBackend* CreateDisplayBackend();

} // namespace Detail
} // namespace PlayOS
