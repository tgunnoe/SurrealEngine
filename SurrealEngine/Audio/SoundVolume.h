#pragma once

#include <algorithm>
#include <cstdint>

// Largest volume any play path may hand to the audio device.
constexpr float SoundVolumeCap = 2.75f;

// Boost factors applied after the curve, see BoostedSoundVolume.
constexpr float DeusExTalkBoost = 2.0f;
constexpr float AmbientBoost = 2.0f;

// Maps the raw volume used by the original games onto the engine's scale.
inline float NormalizeSoundVolume(float v)
{
	if (v >= 8.0f)
		return 0.8f;
	return (v - 1.0f) * 0.25f + 1.0f;
}

// final = min(f(v) * boost, cap)
inline float BoostedSoundVolume(float v, float boost)
{
	return std::min(NormalizeSoundVolume(v) * boost, SoundVolumeCap);
}

// Scales an already-boosted talk volume by an Actor.SetInstantSpeechVolume value (0-255).
inline float SpeechScaledVolume(float volume, uint8_t speechVolume)
{
	return std::min(volume * (speechVolume / 255.0f), SoundVolumeCap);
}
