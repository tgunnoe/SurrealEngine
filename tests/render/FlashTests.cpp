#include "Precomp.h"
#include "TestRunner.h"
#include "RenderDevice/RenderDevice.h"
#include <cmath>

// The device draws the flash in half the script's unit (neutral 0.5, not 1). These tests cover the
// values the renderer locks the device with and the vec4 inequality that decides whether a quad is drawn.

namespace
{
	bool SameComponents(const vec4& a, const vec4& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
	}

	bool Near(float a, float b)
	{
		return std::fabs(a - b) <= 1e-6f;
	}
}

TEST(Vec4InequalitySeesW)
{
	CHECK(vec4(1.0f, 2.0f, 3.0f, 4.0f) != vec4(1.0f, 2.0f, 3.0f, 5.0f));
	CHECK(vec4(1.0f, 2.0f, 3.0f, 4.0f) != vec4(1.0f, 2.0f, 3.0f, -4.0f));
	CHECK(dvec4(1.0, 2.0, 3.0, 4.0) != dvec4(1.0, 2.0, 3.0, 5.0));
	CHECK(dvec4(1.0, 2.0, 3.0, 4.0) != dvec4(1.0, 2.0, 3.0, -4.0));
}

TEST(Vec4InequalityFalseForEqual)
{
	CHECK(!(vec4(0.0f, 0.0f, 0.0f, 0.0f) != vec4(0.0f, 0.0f, 0.0f, 0.0f)));
	CHECK(!(vec4(0.5f, 0.5f, 0.5f, 0.0f) != vec4(0.5f, 0.5f, 0.5f, 0.0f)));
	CHECK(!(vec4(1.0f, 2.0f, 3.0f, 4.0f) != vec4(1.0f, 2.0f, 3.0f, 4.0f)));
	CHECK(!(dvec4(1.0, 2.0, 3.0, 4.0) != dvec4(1.0, 2.0, 3.0, 4.0)));
}

TEST(FlashLockAtRestIsNeutral)
{
	FlashLockValues lock = ComputeFlashLock(vec3(1.0f), vec3(0.0f));
	CHECK(SameComponents(lock.Scale, FlashNeutralScale()));
	CHECK(SameComponents(lock.Fog, FlashNeutralFog()));
}

TEST(FlashLockHalvesScriptScale)
{
	FlashLockValues lock = ComputeFlashLock(vec3(0.61f), vec3(0.2f, 0.1f, 0.05f));
	CHECK(Near(lock.Scale.x, 0.305f) && Near(lock.Scale.y, 0.305f) && Near(lock.Scale.z, 0.305f));
	CHECK(lock.Scale.w == 0.0f);
	CHECK(lock.Fog.x == 0.2f && lock.Fog.y == 0.1f && lock.Fog.z == 0.05f && lock.Fog.w == 0.0f);
}

TEST(FlashLockNoPlayerDefaultIsNeutral)
{
	FlashLockValues lock = ComputeFlashLockNoPlayer();
	CHECK(FlashIsNeutral(lock.Scale, lock.Fog));
}
