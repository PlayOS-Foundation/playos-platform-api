#include "playos/audio.h"

#include "backends/audio_backend.h"

#include <memory>

namespace PlayOS {
namespace {

std::unique_ptr<Detail::IAudioBackend> g_backend;

Detail::IAudioBackend* backend() {
    if (!g_backend) {
        g_backend.reset(Detail::CreateAudioBackend());
    }
    return g_backend.get();
}

} // namespace

namespace Audio {

float GetMasterVolume() {
    return backend()->GetMasterVolume();
}

void SetMasterVolume(float volume) {
    backend()->SetMasterVolume(volume);
}

bool IsMuted() {
    return backend()->IsMuted();
}

void SetMuted(bool muted) {
    backend()->SetMuted(muted);
}

} // namespace Audio
} // namespace PlayOS
