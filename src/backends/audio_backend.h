// Internal audio backend interface. Each platform/engine implements one
// backend; the public PlayOS::Audio API (audio.cpp) delegates to it.
#pragma once

namespace PlayOS {
namespace Detail {

class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    // Master volume in [0.0, 1.0].
    virtual float GetMasterVolume() = 0;

    // Set master volume. Backend clamps to [0.0, 1.0].
    virtual void SetMasterVolume(float volume) = 0;

    // True when muted.
    virtual bool IsMuted() = 0;

    // Mute or unmute.
    virtual void SetMuted(bool muted) = 0;
};

// Creates the platform-selected audio backend.
IAudioBackend* CreateAudioBackend();

} // namespace Detail
} // namespace PlayOS
