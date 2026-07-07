#include "playos/lifecycle.h"

#include "playos/input.h"

namespace PlayOS {
namespace Lifecycle {

bool Init() {
    // Prime the input backend and edge-detection state.
    Input::Update();
    return true;
}

void Update() {
    Input::Update();
}

void Shutdown() {
    // No global resources to release yet.
}

} // namespace Lifecycle
} // namespace PlayOS
