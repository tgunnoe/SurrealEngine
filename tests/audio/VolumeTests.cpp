#include "Precomp.h"
#include "TestRunner.h"
#include "Audio/SoundVolume.h"
#include <cmath>

// Loudness itself cannot be measured: the harness has no audio capture. Measuring the
// rendered output of these paths is deferred work; these tests pin the mapping only.
//
// The expectations are written from the OpenAL source gain model (AL_GAIN 1.0 is
// unattenuated, AL_MAX_GAIN lies in [0, 1]) and from hand-computed anchors, not from the
// implementation's formulas, so a wrong formula cannot make its own test pass.

namespace
{
	constexpr int SweepSteps = 320; // [0, 16] in steps of 0.05, indexed to avoid float accumulation
	float SweepValue(int i) { return i * 0.05f; }

	// Boost factors in ascending order, all at least 1, spanning the documented 2.0.
	constexpr float Boosts[] = { 1.0f, 1.25f, 1.5f, 2.0f, 3.0f, 4.0f };
	constexpr int BoostCount = sizeof(Boosts) / sizeof(Boosts[0]);

	// Every value Actor.SetInstantSpeechVolume can pass.
	constexpr int SpeechByteCount = 256;
	uint8_t SpeechByte(int i) { return (uint8_t)i; }
}

TEST(AudioVolumeCapIsUnity)
{
	CHECK_EQ(SoundVolumeCap, 1.0f);
}

TEST(AudioVolumeAnchors)
{
	CHECK_EQ(NormalizeSoundVolume(-1.0f), 0.0f);
	CHECK_EQ(NormalizeSoundVolume(0.0f), 0.0f);
	CHECK_EQ(NormalizeSoundVolume(0.25f), 0.25f);
	CHECK_EQ(NormalizeSoundVolume(0.5f), 0.5f);
	CHECK_EQ(NormalizeSoundVolume(1.0f), 1.0f);
	CHECK_EQ(NormalizeSoundVolume(2.0f), 1.0f);
	CHECK_EQ(NormalizeSoundVolume(7.9f), 1.0f);
	CHECK_EQ(NormalizeSoundVolume(8.0f), 1.0f);
	CHECK_EQ(NormalizeSoundVolume(16.0f), 1.0f);
	CHECK_EQ(NormalizeSoundVolume(255.0f), 1.0f);
}

TEST(AudioVolumeIdentityBelowUnity)
{
	// Multiples of 1/16 are exact in binary, so the identity can be compared exactly.
	for (int i = 0; i <= 16; i++)
	{
		float v = i / 16.0f;
		CHECK_EQ(NormalizeSoundVolume(v), v);
	}
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
	// The documented factors; the anchors below use them as literals.
	CHECK_EQ(DeusExTalkBoost, 2.0f);
	CHECK_EQ(AmbientBoost, 2.0f);

	// Hand-computed anchors for a boost of 2.
	CHECK_EQ(BoostedSoundVolume(0.0f, 2.0f), 0.0f);
	CHECK_EQ(BoostedSoundVolume(0.25f, 2.0f), 0.5f);
	CHECK_EQ(BoostedSoundVolume(0.5f, 2.0f), 1.0f);
	CHECK_EQ(BoostedSoundVolume(0.75f, 2.0f), 1.0f);
	CHECK_EQ(BoostedSoundVolume(1.0f, 2.0f), 1.0f);
	CHECK_EQ(BoostedSoundVolume(16.0f, 2.0f), 1.0f);

	// A boost of 1 changes nothing.
	for (int i = 0; i <= SweepSteps; i++)
	{
		float v = SweepValue(i);
		CHECK_EQ(BoostedSoundVolume(v, 1.0f), NormalizeSoundVolume(v));
	}
}

TEST(AudioVolumeBoostBounds)
{
	for (float boost : Boosts)
	{
		for (int i = 0; i <= SweepSteps; i++)
		{
			float v = SweepValue(i);
			float boosted = BoostedSoundVolume(v, boost);
			CHECK_MSG(boosted <= SoundVolumeCap, "boost " + std::to_string(boost) + " of " + std::to_string(v) + " exceeds the cap");
			CHECK_MSG(boosted >= NormalizeSoundVolume(v), "boost " + std::to_string(boost) + " of " + std::to_string(v) + " is quieter than no boost");
		}
	}
}

TEST(AudioVolumeBoostOrder)
{
	for (float boost : Boosts)
	{
		for (int i = 1; i <= SweepSteps; i++)
		{
			CHECK_MSG(BoostedSoundVolume(SweepValue(i), boost) >= BoostedSoundVolume(SweepValue(i - 1), boost),
				"boosted volume drops between " + std::to_string(SweepValue(i - 1)) + " and " + std::to_string(SweepValue(i)));
		}
	}

	for (int i = 0; i <= SweepSteps; i++)
	{
		for (int b = 1; b < BoostCount; b++)
		{
			CHECK_MSG(BoostedSoundVolume(SweepValue(i), Boosts[b]) >= BoostedSoundVolume(SweepValue(i), Boosts[b - 1]),
				"volume " + std::to_string(SweepValue(i)) + " drops when the boost rises to " + std::to_string(Boosts[b]));
		}
	}
}

TEST(AudioSpeechVolumeScales)
{
	CHECK_EQ(SpeechScaledVolume(0.75f, 255), 0.75f);
	CHECK_EQ(SpeechScaledVolume(1.0f, 255), 1.0f);
	CHECK_EQ(SpeechScaledVolume(0.75f, 0), 0.0f);
	CHECK(std::fabs(SpeechScaledVolume(1.0f, 51) - 0.2f) < 1e-6f);
	CHECK(std::fabs(SpeechScaledVolume(0.5f, 102) - 0.2f) < 1e-6f);
	CHECK_EQ(SpeechScaledVolume(SoundVolumeCap, 255), SoundVolumeCap);
	CHECK(SpeechScaledVolume(100.0f, 255) <= SoundVolumeCap);
}

TEST(AudioSpeechVolumeOrder)
{
	// Stored volumes are at most the cap, which is 1; multiples of 1/16 are exact in binary.
	for (int s = 0; s <= 16; s++)
	{
		float stored = s / 16.0f;
		CHECK_EQ(SpeechScaledVolume(stored, 0), 0.0f);
		CHECK_EQ(SpeechScaledVolume(stored, 255), stored);
		for (int i = 1; i < SpeechByteCount; i++)
		{
			float prev = SpeechScaledVolume(stored, SpeechByte(i - 1));
			float cur = SpeechScaledVolume(stored, SpeechByte(i));
			CHECK_MSG(cur >= prev, "speech volume drops between bytes " + std::to_string(i - 1) + " and " + std::to_string(i));
			CHECK_MSG(cur <= stored, "speech volume exceeds its input at byte " + std::to_string(i));
		}
	}

	// A louder stored volume never plays quieter at the same speech byte.
	for (int i = 0; i < SpeechByteCount; i++)
	{
		for (int s = 1; s <= 16; s++)
		{
			CHECK(SpeechScaledVolume(s / 16.0f, SpeechByte(i)) >= SpeechScaledVolume((s - 1) / 16.0f, SpeechByte(i)));
		}
	}
}

TEST(AudioSpeechVolumeComposesWithTalkBoost)
{
	// The device applies the speech byte to the stored, already-boosted talk volume.
	CHECK_EQ(SpeechScaledVolume(BoostedSoundVolume(0.25f, DeusExTalkBoost), 255), 0.5f);
	CHECK_EQ(SpeechScaledVolume(BoostedSoundVolume(0.25f, DeusExTalkBoost), 0), 0.0f);
	CHECK_EQ(SpeechScaledVolume(BoostedSoundVolume(1.0f, DeusExTalkBoost), 255), 1.0f);
	CHECK_EQ(SpeechScaledVolume(BoostedSoundVolume(16.0f, DeusExTalkBoost), 255), 1.0f);

	for (int speech : { 0, 1, 64, 128, 254, 255 })
	{
		for (int i = 0; i <= SweepSteps; i++)
		{
			float stored = BoostedSoundVolume(SweepValue(i), DeusExTalkBoost);
			float scaled = SpeechScaledVolume(stored, SpeechByte(speech));
			CHECK_MSG(scaled <= SoundVolumeCap, "speech byte " + std::to_string(speech) + " exceeds the cap at " + std::to_string(SweepValue(i)));
			CHECK_MSG(scaled <= stored, "speech byte " + std::to_string(speech) + " amplifies the stored volume at " + std::to_string(SweepValue(i)));
			if (i > 0)
			{
				float prevStored = BoostedSoundVolume(SweepValue(i - 1), DeusExTalkBoost);
				CHECK_MSG(scaled >= SpeechScaledVolume(prevStored, SpeechByte(speech)),
					"talk volume drops between " + std::to_string(SweepValue(i - 1)) + " and " + std::to_string(SweepValue(i)) + " at speech byte " + std::to_string(speech));
			}
		}
	}
}
