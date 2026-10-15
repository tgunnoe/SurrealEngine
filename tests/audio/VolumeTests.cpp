#include "Precomp.h"
#include "TestRunner.h"
#include "Audio/SoundVolume.h"
#include <cmath>

// Loudness itself cannot be measured: the harness has no audio capture. Measuring the
// rendered output of these paths is deferred work; these tests pin the mapping only.

namespace
{
	constexpr int SweepSteps = 320; // [0, 16] in steps of 0.05, indexed to avoid float accumulation
	float SweepValue(int i) { return i * 0.05f; }
}

TEST(AudioVolumeAnchors)
{
	CHECK_EQ(NormalizeSoundVolume(0.0f), 0.0f);
	CHECK_EQ(NormalizeSoundVolume(1.0f), 1.0f);
	CHECK(NormalizeSoundVolume(16.0f) <= SoundVolumeCap);
}

TEST(AudioVolumeMonotonic)
{
	for (int i = 1; i <= SweepSteps; i++)
	{
		float prev = NormalizeSoundVolume(SweepValue(i - 1));
		float cur = NormalizeSoundVolume(SweepValue(i));
		CHECK_MSG(cur >= prev, "volume drops between " + std::to_string(SweepValue(i - 1)) + " and " + std::to_string(SweepValue(i)));
	}
}

TEST(AudioVolumeContinuous)
{
	// The curve's steepest slope is 1 per unit, so adjacent samples differ by at most 0.05.
	for (int i = 1; i <= SweepSteps; i++)
	{
		float step = std::fabs(NormalizeSoundVolume(SweepValue(i)) - NormalizeSoundVolume(SweepValue(i - 1)));
		CHECK_MSG(step < 0.06f, "jump of " + std::to_string(step) + " at " + std::to_string(SweepValue(i)));
	}
}

TEST(AudioVolumeBoostComposes)
{
	for (float boost : { DeusExTalkBoost, AmbientBoost })
	{
		for (int i = 0; i <= SweepSteps; i++)
		{
			float v = SweepValue(i);
			float expected = std::min(NormalizeSoundVolume(v) * boost, SoundVolumeCap);
			CHECK_EQ(BoostedSoundVolume(v, boost), expected);
			CHECK(BoostedSoundVolume(v, boost) <= SoundVolumeCap);
		}
	}
	CHECK_EQ(BoostedSoundVolume(0.5f, 2.0f), 1.0f);
	CHECK_EQ(BoostedSoundVolume(16.0f, 2.0f), SoundVolumeCap);
}

TEST(AudioSpeechVolumeScales)
{
	CHECK_EQ(SpeechScaledVolume(1.5f, 255), 1.5f);
	CHECK_EQ(SpeechScaledVolume(1.5f, 0), 0.0f);
	CHECK(std::fabs(SpeechScaledVolume(1.0f, 51) - 0.2f) < 1e-6f);
	CHECK_EQ(SpeechScaledVolume(SoundVolumeCap, 255), SoundVolumeCap);
	CHECK(SpeechScaledVolume(100.0f, 255) <= SoundVolumeCap);
}

TEST(AudioSpeechVolumeComposesWithTalkBoost)
{
	// The device applies the speech multiplier to the stored, already-boosted talk volume.
	for (int i = 0; i <= SweepSteps; i++)
	{
		float stored = BoostedSoundVolume(SweepValue(i), DeusExTalkBoost);
		for (int speech : { 0, 64, 128, 255 })
		{
			float scaled = SpeechScaledVolume(stored, (uint8_t)speech);
			CHECK(scaled <= SoundVolumeCap);
			CHECK(scaled <= stored + 1e-6f);
			CHECK(std::fabs(scaled - stored * (speech / 255.0f)) < 1e-5f);
		}
	}
}
