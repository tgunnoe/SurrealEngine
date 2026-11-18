#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Engine/Actors/Pawn/UPawn_AIMath.h"
#include <cmath>
#include <limits>
#include <string>
#include <vector>

// The running total of the seconds a pawn has fallen in a WaitForLanding, and the one step at which it
// first reaches the threshold and LongFall is raised. The steps used here are exact binary fractions, so
// the running totals are exact in float and a total can be compared with the threshold without a margin.

namespace
{
	// Applies a step of the given size count times from a total of zero and lists the steps that raised.
	std::vector<int> RaisedSteps(float step, int count, float threshold, float* total = nullptr)
	{
		std::vector<int> raised;
		float fallen = 0.0f;
		for (int i = 1; i <= count; i++)
		{
			AIMath::FallStep next = AIMath::AdvanceFall(fallen, step, threshold);
			fallen = next.fallen;
			if (next.raise)
				raised.push_back(i);
		}
		if (total)
			*total = fallen;
		return raised;
	}
}

TEST(AILongFallRaisesOnceWhenTheFallCrossesTheThreshold)
{
	float total = 0.0f;
	std::vector<int> raised = RaisedSteps(0.25f, 12, 2.0f, &total);
	CHECK_EQ(raised.size(), (size_t)1);
	CHECK_EQ(raised[0], 8);
	CHECK_EQ(total, 3.0f);
}

TEST(AILongFallDoesNotRaiseBeforeTheThreshold)
{
	for (float step : { 0.0078125f, 0.0625f, 0.25f, 0.5f, 1.0f })
	{
		// Every count of steps whose total is still under two seconds.
		int under = (int)std::ceil(2.0f / step) - 1;
		CHECK_MSG(RaisedSteps(step, under, 2.0f).empty(), "a raise before two seconds with steps of " + std::to_string(step));
	}

	AIMath::FallStep single = AIMath::AdvanceFall(0.0f, 1.9921875f, 2.0f);
	CHECK(!single.raise);
}

TEST(AILongFallRaiseDoesNotDependOnTheStepSize)
{
	for (float step : { 0.0078125f, 0.0625f, 0.25f, 0.5f, 1.0f, 2.0f, 3.0f })
	{
		int firstAtThreshold = (int)std::ceil(2.0f / step);
		int count = step == 3.0f ? 3 : (int)std::ceil(8.0f / step);
		std::vector<int> raised = RaisedSteps(step, count, 2.0f);
		std::string where = "steps of " + std::to_string(step) + ": ";
		CHECK_MSG(raised.size() == 1, where + std::to_string(raised.size()) + " raises, expected one");
		CHECK_MSG(raised[0] == firstAtThreshold, where + "the raise came on step " + std::to_string(raised[0]) + ", expected " + std::to_string(firstAtThreshold));
	}
}

TEST(AILongFallIgnoresNegativeAndNonFiniteElapsed)
{
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	for (float fallen : { 0.0f, 1.5f })
	{
		for (float elapsed : { -1.0f, -0.0f, 0.0f, nan, inf, -inf })
		{
			AIMath::FallStep step = AIMath::AdvanceFall(fallen, elapsed, 2.0f);
			CHECK_MSG(step.fallen == fallen, "the total changed from " + std::to_string(fallen) + " for elapsed " + std::to_string(elapsed));
			CHECK_MSG(!step.raise, "a raise for elapsed " + std::to_string(elapsed));
		}
	}
}

TEST(AILongFallThresholdIsTwoSeconds)
{
	CHECK_EQ(AIMath::LongFallSeconds, 2.0f);
}

TEST(AILongFallRaisesWhereAnotherThresholdIsReached)
{
	std::vector<int> raised = RaisedSteps(0.25f, 8, 1.0f);
	CHECK_EQ(raised.size(), (size_t)1);
	CHECK_EQ(raised[0], 4);
}
