// Raylib audio backend — wraps Raylib's audio API behind IAudioBackend.
// Works on Windows, Linux, macOS, Android, and Web.

#include "backends/audio_backend.h"

#include "raylib.h"

#include <algorithm>

namespace PlayOS {
namespace Detail {
namespace {

class RaylibAudioBackend : public IAudioBackend {
public:
    float GetMasterVolume() override {
        if (!m_initialized && !tryInit()) return 0.0f;
        return m_volume;
    }

    void SetMasterVolume(float volume) override {
        if (!m_initialized && !tryInit()) return;
        m_volume = std::clamp(volume, 0.0f, 1.0f);
        ::SetMasterVolume(m_muted ? 0.0f : m_volume);
    }

    bool IsMuted() override {
        return m_muted;
    }

    void SetMuted(bool muted) override {
        if (!m_initialized && !tryInit()) return;
        m_muted = muted;
        ::SetMasterVolume(muted ? 0.0f : m_volume);
    }

private:
    bool tryInit() {
        if (IsAudioDeviceReady()) {
            m_initialized = true;
            return true;
        }
        // InitAudioDevice may fail on headless / CI — that's fine.
        InitAudioDevice();
        m_initialized = IsAudioDeviceReady();
        return m_initialized;
    }

    bool m_initialized = false;
    float m_volume = 1.0f;
    bool m_muted = false;
};

} // namespace

IAudioBackend* CreateAudioBackend() {
    return new RaylibAudioBackend();
}

} // namespace Detail
} // namespace PlayOS
