#include "Precomp.h"
#include "TestRunner.h"
#include "Render/VisibleFrame.h"
#include "Math/mat.h"
#include <cmath>
#include <random>

namespace
{
	// The training map's stairs: four floor mirrors facing up, and the eye of the player on the top step.
	constexpr float StairHeights[4] = { -128.0f, -144.0f, -160.0f, -176.0f };
	const vec3 StairEye(1494.0f, 834.0f, -62.1f);

	bool Near(float a, float b, float eps)
	{
		return std::fabs(a - b) <= eps;
	}

	bool NearV(const vec3& a, const vec3& b, float eps)
	{
		return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
	}

	float Distance(const vec4& plane, const vec3& point)
	{
		return dot(plane, vec4(point, 1.0f));
	}

	vec3 RandomUnit(std::mt19937& rng)
	{
		std::uniform_real_distribution<float> u(-1.0f, 1.0f);
		for (;;)
		{
			vec3 n(u(rng), u(rng), u(rng));
			if (length(n) > 0.2f)
				return normalize(n);
		}
	}

	vec3 RandomPoint(std::mt19937& rng, float halfWidth)
	{
		std::uniform_real_distribution<float> u(-halfWidth, halfWidth);
		return vec3(u(rng), u(rng), u(rng));
	}

	vec4 StairPlane(float height)
	{
		return MirrorSurfacePlane(vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, height));
	}
}

TEST(ReflectPointKeepsPointsOnThePlane)
{
	std::mt19937 rng(98);
	std::uniform_real_distribution<float> u(-500.0f, 500.0f);
	for (int i = 0; i < 200; i++)
	{
		vec3 n = RandomUnit(rng);
		vec3 v = RandomPoint(rng, 1000.0f);
		vec3 a = normalize(cross(n, std::fabs(n.x) < 0.9f ? vec3(1.0f, 0.0f, 0.0f) : vec3(0.0f, 1.0f, 0.0f)));
		vec3 b = cross(n, a);
		vec3 p = v + a * u(rng) + b * u(rng);
		CHECK(NearV(ReflectPoint(p, MirrorSurfacePlane(n, v)), p, 5e-2f));
	}
}

TEST(ReflectPointIsAnInvolution)
{
	std::mt19937 rng(99);
	for (int i = 0; i < 200; i++)
	{
		vec4 plane = MirrorSurfacePlane(RandomUnit(rng), RandomPoint(rng, 1000.0f));
		vec3 p = RandomPoint(rng, 1000.0f);
		CHECK(NearV(ReflectPoint(ReflectPoint(p, plane), plane), p, 5e-2f));
	}
}

TEST(MirrorFrameDepthOnePlaneIsUnchanged)
{
	for (float z : StairHeights)
	{
		vec4 plane = MirrorFrameViewOf(StairEye, StairEye, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, z)).ClipPlane;
		vec4 expected = MirrorClipPlane(vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, z), StairEye);
		CHECK(plane.x == expected.x && plane.y == expected.y && plane.z == expected.z && plane.w == expected.w);
		CHECK(Near(plane.x, 0.0f, 1e-4f) && Near(plane.y, 0.0f, 1e-4f) && Near(plane.z, 1.0f, 1e-4f) && Near(plane.w, -z - 1.0f, 1e-4f));
	}
}

TEST(MirrorFrameKeepsTheZoneLocation)
{
	vec3 eye = StairEye;
	for (float z : StairHeights)
	{
		MirrorFrameView view = MirrorFrameViewOf(eye, StairEye, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, z));
		CHECK(view.ZoneLocation.x == StairEye.x && view.ZoneLocation.y == StairEye.y && view.ZoneLocation.z == StairEye.z);
		eye = view.Eye;
	}
}

TEST(MirrorPortalInFrontOfTheEyeIsPortalled)
{
	CHECK(MirrorPortalFacesEye(vec4(0.0f, 0.0f, 1.0f, 128.0f), StairEye));
	CHECK(MirrorPortalFacesEye(vec4(1.0f, 0.0f, 0.0f, -10.0f), vec3(50.0f, 3.0f, 4.0f)));
	CHECK(MirrorPortalFacesEye(vec4(0.0f, -1.0f, 0.0f, -62.0f), vec3(0.0f, -300.0f, 0.0f)));

	std::mt19937 rng(11);
	std::uniform_real_distribution<float> offset(1.0f, 501.0f);
	for (int i = 0; i < 200; i++)
	{
		vec3 n = RandomUnit(rng);
		vec3 q = RandomPoint(rng, 1000.0f);
		CHECK(MirrorPortalFacesEye(MirrorSurfacePlane(n, q), q + n * offset(rng)));
	}
}

TEST(FacingMirrorsStillNest)
{
	// Two walls facing each other, the viewer between them: each is seen from the other's reflection.
	vec3 e(300.0f, 0.0f, 0.0f);
	vec3 n0(1.0f, 0.0f, 0.0f), q0(0.0f, 0.0f, 0.0f);
	vec3 n1(-1.0f, 0.0f, 0.0f), q1(1000.0f, 0.0f, 0.0f);
	MirrorFrameView first = MirrorFrameViewOf(e, e, n0, q0);
	CHECK(MirrorPortalFacesEye(MirrorSurfacePlane(n1, q1), first.Eye));
	MirrorFrameView second = MirrorFrameViewOf(first.Eye, e, n1, q1);
	CHECK(MirrorPortalFacesEye(MirrorSurfacePlane(n0, q0), second.Eye));

	// Two perpendicular walls, the viewer in front of both: each is seen from the other's reflection.
	vec3 c(300.0f, 200.0f, 0.0f);
	vec3 wallX(1.0f, 0.0f, 0.0f), wallY(0.0f, 1.0f, 0.0f), origin(0.0f, 0.0f, 0.0f);
	CHECK(MirrorPortalFacesEye(MirrorSurfacePlane(wallY, origin), MirrorFrameViewOf(c, c, wallX, origin).Eye));
	CHECK(MirrorPortalFacesEye(MirrorSurfacePlane(wallX, origin), MirrorFrameViewOf(c, c, wallY, origin).Eye));
}

TEST(ReflectPointAboutPlane)
{
	// The first case is the training stair, the player's eye reflected in the second tread.
	CHECK(NearV(ReflectPoint(vec3(1494.0f, 834.0f, -62.1f), MirrorSurfacePlane(vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, -144.0f))), vec3(1494.0f, 834.0f, -225.9f), 1e-2f));
	CHECK(NearV(ReflectPoint(vec3(50.0f, 3.0f, 4.0f), MirrorSurfacePlane(vec3(1.0f, 0.0f, 0.0f), vec3(10.0f, 0.0f, 0.0f))), vec3(-30.0f, 3.0f, 4.0f), 1e-2f));
	CHECK(NearV(ReflectPoint(vec3(5.0f, 5.0f, 1.0f), MirrorSurfacePlane(vec3(0.6f, 0.8f, 0.0f), vec3(0.0f, 0.0f, 0.0f))), vec3(-3.4f, -6.2f, 1.0f), 1e-2f));
	CHECK(NearV(ReflectPoint(vec3(7.0f, -300.0f, 1.0f), MirrorSurfacePlane(vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, -62.0f, 0.0f))), vec3(7.0f, 176.0f, 1.0f), 1e-2f));
}

TEST(ReflectPointMatchesTheMirrorMatrix)
{
	std::mt19937 rng(97);
	int checked = 0;
	for (int i = 0; i < 200; i++)
	{
		vec3 n = RandomUnit(rng);
		vec3 v = RandomPoint(rng, 1000.0f);
		vec3 p = RandomPoint(rng, 1000.0f);
		vec4 plane = MirrorSurfacePlane(n, v);
		if (std::fabs(Distance(plane, p)) < 5.0f)
			continue;

		mat4 m = mat4::translate(v) * mat4::mirror(n) * mat4::translate(-v);
		vec4 r = m * vec4(p, 1.0f);
		CHECK(NearV(vec3(r.x, r.y, r.z), ReflectPoint(p, plane), 2e-2f));
		checked++;
	}
	CHECK(checked > 150);
}

TEST(MirrorFrameEyeIsTheReflectedEye)
{
	MirrorFrameView tread = MirrorFrameViewOf(StairEye, StairEye, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, -144.0f));
	CHECK(NearV(tread.Eye, vec3(1494.0f, 834.0f, -225.9f), 1e-2f));

	vec3 wallEye(50.0f, 3.0f, 4.0f);
	MirrorFrameView wall = MirrorFrameViewOf(wallEye, wallEye, vec3(1.0f, 0.0f, 0.0f), vec3(10.0f, 0.0f, 0.0f));
	CHECK(NearV(wall.Eye, vec3(-30.0f, 3.0f, 4.0f), 1e-2f));
}

TEST(MirrorFrameEyeFollowsTheViewMatrices)
{
	// Three oblique planes, none symmetric to another, so the order they are applied in shows.
	struct Mirror { vec3 normal, point; };
	const Mirror path[3] =
	{
		{ normalize(vec3(0.6f, 0.8f, 0.0f)), vec3(100.0f, 50.0f, 0.0f) },
		{ normalize(vec3(0.0f, 0.28f, 0.96f)), vec3(-200.0f, 0.0f, 300.0f) },
		{ normalize(vec3(-0.8f, 0.36f, 0.48f)), vec3(400.0f, -100.0f, 50.0f) }
	};
	const vec3 root(120.0f, -60.0f, 170.0f);

	// A mirror frame's view matrix is its parent's times the reflection about its mirror, so the
	// product below maps the last frame's eye back to the root eye.
	vec3 eye = root;
	mat4 chain = mat4::identity();
	for (const Mirror& m : path)
	{
		eye = MirrorFrameViewOf(eye, root, m.normal, m.point).Eye;
		chain = chain * (mat4::translate(m.point) * mat4::mirror(m.normal) * mat4::translate(-m.point));
	}
	vec4 back = chain * vec4(eye, 1.0f);
	CHECK(NearV(vec3(back.x, back.y, back.z), root, 5e-2f));

	// Reflecting in the opposite order must not satisfy it, or these planes could not tell the orders apart.
	vec3 reversed = root;
	for (int i = 2; i >= 0; i--)
		reversed = ReflectPoint(reversed, MirrorSurfacePlane(path[i].normal, path[i].point));
	vec4 wrong = chain * vec4(reversed, 1.0f);
	CHECK(!NearV(vec3(wrong.x, wrong.y, wrong.z), root, 1.0f));
}

TEST(MirrorFrameClipPlaneFacesTheParentEye)
{
	// The stair in two links: the tread's frame, then the landing seen from the tread frame's eye.
	MirrorFrameView tread = MirrorFrameViewOf(StairEye, StairEye, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, -144.0f));
	MirrorFrameView landing = MirrorFrameViewOf(tread.Eye, StairEye, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, -128.0f));
	CHECK(Distance(landing.ClipPlane, tread.Eye) > 0.0f);
	CHECK(Distance(landing.ClipPlane, vec3(1494.0f, 834.0f, -100.0f)) < 0.0f);
	CHECK(Near(landing.ClipPlane.x, 0.0f, 1e-3f) && Near(landing.ClipPlane.y, 0.0f, 1e-3f) && Near(landing.ClipPlane.z, -1.0f, 1e-3f) && Near(landing.ClipPlane.w, -129.0f, 1e-3f));

	std::mt19937 rng(7);
	int checked = 0;
	for (int i = 0; i < 200; i++)
	{
		vec3 n = RandomUnit(rng);
		vec3 q = RandomPoint(rng, 1000.0f);
		vec3 e = RandomPoint(rng, 1000.0f);
		if (std::fabs(Distance(MirrorSurfacePlane(n, q), e)) < 5.0f)
			continue;

		MirrorFrameView view = MirrorFrameViewOf(e, e, n, q);
		CHECK(Distance(view.ClipPlane, e) > 0.0f);
		CHECK(Distance(view.ClipPlane, view.Eye) < 0.0f);
		checked++;
	}
	CHECK(checked > 150);
}

TEST(MirrorPortalBehindTheEyeIsNotPortalled)
{
	const vec4 landing(0.0f, 0.0f, 1.0f, 128.0f);
	CHECK(!MirrorPortalFacesEye(landing, vec3(1494.0f, 834.0f, -194.0f)));
	CHECK(!MirrorPortalFacesEye(landing, vec3(1494.0f, 834.0f, -128.0f)));
	CHECK(!MirrorPortalFacesEye(vec4(1.0f, 0.0f, 0.0f, -10.0f), vec3(-50.0f, 3.0f, 4.0f)));
	CHECK(!MirrorPortalFacesEye(vec4(0.0f, -1.0f, 0.0f, -62.0f), vec3(0.0f, 0.0f, 0.0f)));
}

TEST(TrainingStairsHaveNoNestedPortal)
{
	for (float z : { -61.9f, -62.1f })
	{
		vec3 eye(1494.0f, 834.0f, z);
		for (float b : StairHeights)
			CHECK(MirrorPortalFacesEye(StairPlane(b), eye));

		// Seen from the eye reflected in any one stair mirror, none of the four faces it.
		for (float a : StairHeights)
		{
			vec3 reflected = MirrorFrameViewOf(eye, eye, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, a)).Eye;
			for (float b : StairHeights)
				CHECK(!MirrorPortalFacesEye(StairPlane(b), reflected));
		}
	}
}
