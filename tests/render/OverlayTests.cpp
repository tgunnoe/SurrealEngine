#include "Precomp.h"
#include "TestRunner.h"
#include "Render/RenderSubsystem.h"

TEST(TimedemoOverlayOffInDeusEx)
{
	CHECK(!ShouldDrawTimedemoStats(false, true));
	CHECK(!ShouldDrawTimedemoStats(false, false));
}

TEST(TimedemoOverlayOnWithTimedemo)
{
	CHECK(ShouldDrawTimedemoStats(true, true));
	CHECK(ShouldDrawTimedemoStats(true, false));
}
