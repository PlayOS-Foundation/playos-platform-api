// PlayOS Platform API — Display.
//
// Query display properties (size, safe area, refresh rate). Required
// capability `display.info` — every conformant target provides this.
// See playos-spec: RFC-0004, book/src/05-capability-model/03-required-capabilities.md.
#pragma once

namespace PlayOS {

struct DisplayInfo {
    int width;              // current display width in pixels
    int height;             // current display height in pixels
    int refreshRate;        // current refresh rate in Hz

    // Safe area insets in pixels (for notches, rounded corners, bezels).
    // On devices without obstructions these are all zero.
    int safeAreaLeft;
    int safeAreaTop;
    int safeAreaRight;
    int safeAreaBottom;
};

namespace Display {

// Returns the current display properties of the primary monitor.
DisplayInfo Current();

} // namespace Display
} // namespace PlayOS
