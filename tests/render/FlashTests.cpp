#include "Precomp.h"
#include "TestRunner.h"
#include "RenderDevice/RenderDevice.h"

// The device draws the flash in half the script's unit (neutral 0.5, not 1). These tests cover the
// values the renderer locks the device with and the vec4 inequality that decides whether a quad is drawn.

TEST(FlashLockNoPlayerDefaultIsNeutral)
{
	FlashLockValues lock = ComputeFlashLockNoPlayer();
	CHECK(FlashIsNeutral(lock.Scale, lock.Fog));
}
