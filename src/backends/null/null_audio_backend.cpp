// Null audio backend — returns defaults when no audio device is available.

#include "backends/audio_backend.h"

namespace PlayOS {
namespace Detail {
namespace {

class NullAudioBackend : public IAudioBackend {
public:
    float GetMasterVolume() override { return 0.5f; }
    void SetMasterVolume(float) override {}
    bool IsMuted() override { return false; }
    void SetMuted(bool) override {}
};

} // namespace

IAudioBackend* CreateAudioBackend() {
    return new NullAudioBackend();
}

} // namespace Detail
} // namespace PlayOS
