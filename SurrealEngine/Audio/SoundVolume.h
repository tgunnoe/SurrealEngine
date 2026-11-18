#pragma once

#include <algorithm>
#include <cstdint>

// Largest volume any play path may hand to the audio device.
//
// The device writes the result as the source's AL_GAIN. AL/al.h documents AL_GAIN as "an
// initial linear gain value (before attenuation is applied)", where 1.0 "means
// unattenuated", and limits AL_MAX_GAIN to the range [0.0 - 1.0]: a single source never
// plays above full scale. The device leaves AL_MAX_GAIN at its default of 1.0, which is
// this cap.
constexpr float SoundVolumeCap = 1.0f;

// Boost factors applied after the curve, see BoostedSoundVolume.
constexpr float DeusExTalkBoost = 2.0f;
constexpr float AmbientBoost = 2.0f;

// Maps the raw volume used by the original games onto the engine's scale.
//
// The originals request volumes well above 1 (16 for interface sounds, 255 in Unreal Gold,
// 2 to 8 for weapons) because their mixer saturated. Read that as "as loud as a single
// source gets", not "that many times louder": the result is the request clamped to
// [0, SoundVolumeCap]. It is the identity up to 1, non-decreasing and continuous, so a
// louder request never plays quieter than a softer one.
//
// Whether the result sounds right cannot be measured yet: the harness has no audio capture.
// Measuring rendered loudness for the Pulse Rifle, minigun and announcer sounds is deferred.
inline float NormalizeSoundVolume(float v)
{
	return std::clamp(v, 0.0f, SoundVolumeCap);
}

// The curve first, then the boost, then the shared cap.
inline float BoostedSoundVolume(float v, float boost)
{
	return std::min(NormalizeSoundVolume(v) * boost, SoundVolumeCap);
}

// Scales an already-boosted talk volume by an Actor.SetInstantSpeechVolume value (0-255).
inline float SpeechScaledVolume(float volume, uint8_t speechVolume)
{
	return std::min(volume * (speechVolume / 255.0f), SoundVolumeCap);
}
