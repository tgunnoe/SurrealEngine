#include "Precomp.h"
#include "TestRunner.h"
#include "Render/RenderSubsystem.h"

TEST(TimedemoOverlayOffInDeusEx)
{
	CHECK(!ShouldDrawTimedemoStats(false, true, false));
	CHECK(!ShouldDrawTimedemoStats(false, false, false));
}

TEST(TimedemoOverlayOnWithTimedemo)
{
	CHECK(ShouldDrawTimedemoStats(true, true, false));
	CHECK(ShouldDrawTimedemoStats(true, false, false));
}

TEST(TimedemoOverlayOnWithRenderStats)
{
	CHECK(ShouldDrawTimedemoStats(false, true, true));
	CHECK(ShouldDrawTimedemoStats(false, false, true));
}
