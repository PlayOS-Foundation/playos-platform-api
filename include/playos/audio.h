// PlayOS Platform API — Audio.
//
// Master volume and mute control. Core (portable) subset per RFC-0004:
// "Audio (volume, basic playback helper)."
// See playos-spec: RFC-0004.
#pragma once

namespace PlayOS {
namespace Audio {

// Master volume in range [0.0, 1.0].
float GetMasterVolume();

// Set master volume. Clamped to [0.0, 1.0].
void SetMasterVolume(float volume);

// True when audio output is muted.
bool IsMuted();

// Mute or unmute audio output.
void SetMuted(bool muted);

} // namespace Audio
} // namespace PlayOS
