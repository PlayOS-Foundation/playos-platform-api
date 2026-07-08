// PlayOS Platform API — Touch.
//
// Multi-touch input. Optional capability `input.touch` — query via
// Capabilities::Has(Capability::Touch) before use.
// See playos-spec: RFC-0003, RFC-0004.
#pragma once

namespace PlayOS {

struct TouchPoint {
    int id;   // unique touch identifier for the duration of the contact
    int x;    // x position in pixels
    int y;    // y position in pixels
};

namespace Touch {

// Number of active touch points this frame.
int PointCount();

// Returns the touch point at the given index (0-based, < PointCount()).
// Behaviour is undefined if index >= PointCount().
TouchPoint GetPoint(int index);

} // namespace Touch
} // namespace PlayOS
