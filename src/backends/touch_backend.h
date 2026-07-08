// Internal touch backend interface. Each platform/engine implements one
// backend; the public PlayOS::Touch API (touch.cpp) delegates to it.
#pragma once

#include "playos/touch.h"

namespace PlayOS {
namespace Detail {

class ITouchBackend {
public:
    virtual ~ITouchBackend() = default;

    // Number of active touch points.
    virtual int PointCount() = 0;

    // Returns the touch point at the given index (0-based).
    virtual TouchPoint GetPoint(int index) = 0;

    // True when a touch-capable device is present. Used by
    // Capabilities::Has(Capability::Touch) at runtime.
    virtual bool Available() const = 0;
};

// Creates the platform-selected touch backend.
ITouchBackend* CreateTouchBackend();

} // namespace Detail
} // namespace PlayOS
