#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Engine/Actors/Pawn/UPawn_AIMath.h"
#include <vector>

// The cap UPawn::TickRotating puts on a pawn's body pitch. A pitch is an unsigned angle of 65536 to the
// turn, so "below level" is 65536 minus the rate and up to 32767 is above it. The band is the pitches no
// further from level than the rate; a desired pitch outside it is brought to the nearest edge on its own
// side, and one inside it is kept.

namespace
{
	const std::vector<int> Rates = { 0, 1, 2048, 3072, 4096, 8192, 16383, 32767 };

	bool InsideBand(int pitch, int rate)
	{
		return pitch <= rate || pitch >= AIMath::AngleUnits - rate;
	}
}

TEST(AIBodyPitchUpwardAboveTheRateIsCappedAtTheRate)
{
	for (int pitch : { 4097, 5000, 16384, 32767 })
		CHECK_EQ(AIMath::CapBodyPitch(pitch, 4096), 4096);
}

TEST(AIBodyPitchUpwardBelowTheRateIsKept)
{
	for (int pitch : { 0, 1, 100, 4095, 4096 })
		CHECK_EQ(AIMath::CapBodyPitch(pitch, 4096), pitch);
}

TEST(AIBodyPitchDownwardBelowTheBandIsCappedAtTheBand)
{
	for (int pitch : { 32768, 40000, 49152, 61439 })
		CHECK_EQ(AIMath::CapBodyPitch(pitch, 4096), 61440);
}

TEST(AIBodyPitchDownwardInsideTheBandIsKept)
{
	for (int pitch : { 61440, 61441, 65000, 65535 })
		CHECK_EQ(AIMath::CapBodyPitch(pitch, 4096), pitch);
}

TEST(AIBodyPitchNeverLeavesTheBandOverTheWholeRange)
{
	for (int rate : Rates)
	{
		for (int pitch = 0; pitch < AIMath::AngleUnits; pitch++)
		{
			int capped = AIMath::CapBodyPitch(pitch, rate);
			CHECK_MSG(capped >= 0 && capped < AIMath::AngleUnits,
				"rate " + std::to_string(rate) + " pitch " + std::to_string(pitch) + " gave " + std::to_string(capped) + ", outside 0 to 65535");
			CHECK_MSG(InsideBand(capped, rate),
				"rate " + std::to_string(rate) + " pitch " + std::to_string(pitch) + " gave " + std::to_string(capped) + ", outside the band");
		}
	}
}

TEST(AIBodyPitchInsideTheBandIsReturnedUnchanged)
{
	for (int rate : Rates)
	{
		for (int pitch = 0; pitch < AIMath::AngleUnits; pitch++)
		{
			if (!InsideBand(pitch, rate))
				continue;
			int capped = AIMath::CapBodyPitch(pitch, rate);
			CHECK_MSG(capped == pitch,
				"rate " + std::to_string(rate) + " pitch " + std::to_string(pitch) + " gave " + std::to_string(capped) + ", not the pitch itself");
		}
	}
}
