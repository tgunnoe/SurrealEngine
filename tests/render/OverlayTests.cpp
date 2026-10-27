#include "Precomp.h"
#include "TestRunner.h"
#include "Render/RenderSubsystem.h"

TEST(TimedemoOverlayOnWithTimedemo)
{
	CHECK(ShouldDrawTimedemoStats(true, true));
	CHECK(ShouldDrawTimedemoStats(true, false));
}
