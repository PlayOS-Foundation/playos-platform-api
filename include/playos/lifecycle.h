// PlayOS Platform API — Lifecycle.
//
// Minimal application lifecycle: initialize the platform, update once per
// frame, and shut down. See playos-spec:
// book/src/08-runtime-architecture/06-application-lifecycle.md.
#pragma once

namespace PlayOS {
namespace Lifecycle {

// Initializes the Platform API (input backend, capability registry).
// Returns false if initialization fails.
bool Init();

// Advances the platform by one frame. Polls input. Call once per frame.
void Update();

// Releases platform resources.
void Shutdown();

} // namespace Lifecycle
} // namespace PlayOS
