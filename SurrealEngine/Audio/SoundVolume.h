#pragma once

#include <algorithm>
#include <cstdint>

// Largest volume any play path may hand to the audio device.
constexpr float SoundVolumeCap = 2.75f;

// Boost factors applied after the curve, see BoostedSoundVolume.
constexpr float DeusExTalkBoost = 2.0f;
constexpr float AmbientBoost = 2.0f;

// Maps the raw volume used by the original games onto the engine's scale.
//
// The originals use volumes well above 1 (announcer sounds are 8 or more) and expect the
// mixer to cope. Up to 1 the curve is the identity; above it, it climbs at a quarter of the
// slope and saturates at SoundVolumeCap. It is non-decreasing and continuous everywhere, so
// a louder request never plays quieter than a softer one.
//
// OpenAL Soft accepts a source AL_MAX_GAIN above 1 (checked against the installed library),
// so the per-source gain set from these values is not clamped to 1 in the device.
//
// Whether the result sounds right cannot be measured yet: the harness has no audio capture.
// Measuring rendered loudness for the Pulse Rifle, minigun and announcer sounds is deferred.
inline float NormalizeSoundVolume(float v)
{
	if (v <= 0.0f)
		return 0.0f;
	if (v <= 1.0f)
		return v;
	return std::min(1.0f + (v - 1.0f) * 0.25f, SoundVolumeCap);
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
