#include "Precomp.h"
#include "TestRunner.h"
#include "Render/VisibleFrame.h"
#include "Render/BspClipper.h"
#include "Math/bbox.h"
#include <cmath>
#include <random>

namespace
{
	bool Near(float a, float b, float eps)
	{
		return std::fabs(a - b) <= eps;
	}

	vec3 Reflected(const vec3& p, const vec3& n)
	{
		float d = dot(n, p);
		return vec3(p.x - 2.0f * d * n.x, p.y - 2.0f * d * n.y, p.z - 2.0f * d * n.z);
	}

	void CheckMirrorMatrixReflects(const vec3& n)
	{
		const vec3 points[] = { vec3(1.0f, 2.0f, 3.0f), vec3(-4.0f, 5.0f, -6.0f), vec3(0.0f, 7.0f, 0.0f), vec3(2.5f, -1.0f, 9.0f) };
		mat4 m = mat4::mirror(n);
		for (const vec3& p : points)
		{
			vec4 r = m * vec4(p, 1.0f);
			vec3 e = Reflected(p, n);
			CHECK(Near(r.x, e.x, 1e-4f) && Near(r.y, e.y, 1e-4f) && Near(r.z, e.z, 1e-4f) && Near(r.w, 1.0f, 1e-4f));
		}
	}

	VisiblePortal MakePortal(bool isMirror, const vec4& plane)
	{
		VisiblePortal portal;
		portal.IsMirror = isMirror;
		portal.MirrorPlane = plane;
		return portal;
	}
}

TEST(MirrorMatrixReflectsXNormal)
{
	CheckMirrorMatrixReflects(vec3(1.0f, 0.0f, 0.0f));
	CheckMirrorMatrixReflects(vec3(-1.0f, 0.0f, 0.0f));
}

TEST(MirrorMatrixReflectsObliqueNormals)
{
	CheckMirrorMatrixReflects(normalize(vec3(0.6f, 0.8f, 0.0f)));
	CheckMirrorMatrixReflects(normalize(vec3(0.975f, 0.0f, 0.221f)));
	CheckMirrorMatrixReflects(normalize(vec3(0.36f, 0.48f, 0.8f)));
}

TEST(MirrorMatrixIsSymmetricInvolution)
{
	std::mt19937 rng(12345);
	std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
	mat4 identity = mat4::identity();
	int checked = 0;
	for (int i = 0; i < 200; i++)
	{
		vec3 n(unit(rng), unit(rng), unit(rng));
		if (length(n) < 0.1f)
			continue;
		n = normalize(n);
		mat4 m = mat4::mirror(n);
		for (int r = 0; r < 3; r++)
		{
			for (int c = 0; c < 3; c++)
				CHECK(Near(m.matrix[r + c * 4], m.matrix[c + r * 4], 1e-6f));
		}
		mat4 twice = m * m;
		for (int k = 0; k < 16; k++)
			CHECK(Near(twice.matrix[k], identity.matrix[k], 1e-4f));
		checked++;
	}
	CHECK(checked > 150);
}

TEST(MirrorMatrixReflectsYAndZNormals)
{
	CheckMirrorMatrixReflects(vec3(0.0f, 1.0f, 0.0f));
	CheckMirrorMatrixReflects(vec3(0.0f, -1.0f, 0.0f));
	CheckMirrorMatrixReflects(vec3(0.0f, 0.0f, 1.0f));
	CheckMirrorMatrixReflects(vec3(0.0f, 0.0f, -1.0f));
}

TEST(MirrorClipPlaneKeepsViewerSide)
{
	struct Case
	{
		vec3 Normal;
		vec3 Point;
		vec3 Viewer;
	};
	// A wall seen from its front, the same wall from behind its stated normal, a floor mirror
	// and a wall that faces away from the origin.
	const Case cases[] =
	{
		{ vec3(1.0f, 0.0f, 0.0f), vec3(10.0f, 0.0f, 0.0f), vec3(50.0f, 3.0f, 4.0f) },
		{ vec3(1.0f, 0.0f, 0.0f), vec3(10.0f, 0.0f, 0.0f), vec3(-50.0f, 3.0f, 4.0f) },
		{ vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 216.0f), vec3(5.0f, 5.0f, 300.0f) },
		{ vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, -62.0f, 0.0f), vec3(7.0f, -300.0f, 1.0f) },
	};
	for (const Case& c : cases)
	{
		vec4 plane = MirrorClipPlane(c.Normal, c.Point, c.Viewer);
		auto distance = [&](const vec3& x) { return plane.x * x.x + plane.y * x.y + plane.z * x.z + plane.w; };

		float side = dot(c.Normal, c.Viewer) - dot(c.Normal, c.Point) >= 0.0f ? 1.0f : -1.0f;
		vec3 toward = c.Normal * side;
		CHECK(distance(c.Viewer) > 0.0f);
		CHECK(distance(c.Point + toward * 10.0f) > 0.0f);
		CHECK(distance(c.Point - toward * 10.0f) < 0.0f);
		CHECK(distance(c.Point) < 0.0f);
	}
}

TEST(MirrorActorBehindPlaneIsCulled)
{
	// Signed distance x - 100, positive in front.
	const vec4 plane(1.0f, 0.0f, 0.0f, -100.0f);
	auto box = [](float x0, float x1) { return BBox(vec3(x0, -10.0f, -10.0f), vec3(x1, 10.0f, 10.0f)); };
	CHECK(MirrorPlaneCullsBox(box(20.0f, 60.0f), plane));
	CHECK(!MirrorPlaneCullsBox(box(120.0f, 160.0f), plane));
	CHECK(!MirrorPlaneCullsBox(box(80.0f, 120.0f), plane));
	CHECK(!MirrorPlaneCullsBox(box(60.0f, 100.0f), plane));
}

TEST(BspClipperCullsBehindAPortalPlane)
{
	// The camera is at the origin looking along +z, and the plane keeps what lies beyond z = 100.
	BspClipper clipper;
	mat4 view = mat4::identity();
	mat4 projection = mat4::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 32768.0f, handedness::left, clipzrange::zero_positive_w);
	clipper.Setup(projection * view, {}, vec4(0.0f, 0.0f, 1.0f, -100.0f));

	const vec3 behind[3] = { vec3(-50.0f, -50.0f, 50.0f), vec3(50.0f, -50.0f, 50.0f), vec3(0.0f, 50.0f, 50.0f) };
	const vec3 beyond[3] = { vec3(-50.0f, -50.0f, 150.0f), vec3(50.0f, -50.0f, 150.0f), vec3(0.0f, 50.0f, 150.0f) };
	CHECK(!clipper.CheckSurface(behind, 3, true));
	CHECK(clipper.CheckSurface(beyond, 3, true));
}

TEST(MirrorPortalPerPlane)
{
	const vec4 planeA(1.0f, 0.0f, 0.0f, -10.0f);
	const vec4 planeB(0.0f, 1.0f, 0.0f, -20.0f);
	Array<VisiblePortal> portals;
	portals.push_back(MakePortal(true, planeA));
	CHECK_EQ(FindMirrorPortalToJoin(portals, planeB), -1);

	portals.push_back(MakePortal(true, planeB));
	CHECK_EQ(FindMirrorPortalToJoin(portals, planeB), 1);
	CHECK_EQ(FindMirrorPortalToJoin(portals, planeA), 0);
}

TEST(MirrorPortalSamePlaneMerges)
{
	const vec4 planeA(0.0f, 0.0f, 1.0f, -216.0f);
	Array<VisiblePortal> portals;
	CHECK_EQ(FindMirrorPortalToJoin(portals, planeA), -1);

	// A sky or warp portal is not a mirror and is skipped, however its plane member reads.
	portals.push_back(MakePortal(false, planeA));
	portals.push_back(MakePortal(true, planeA));
	CHECK_EQ(FindMirrorPortalToJoin(portals, vec4(0.0f, 0.0f, 1.0f, -216.0f)), 1);
	CHECK_EQ(FindMirrorPortalToJoin(portals, vec4(0.0f, 0.0f, 1.0f, -216.2f)), 1);
}

TEST(MirrorPortalDifferentPlanesDoNotMerge)
{
	// Parallel planes 16 units apart, perpendicular planes, and opposite normals are all different planes.
	struct Case
	{
		vec4 Held;
		vec4 Query;
	};
	const Case cases[] =
	{
		{ vec4(0.0f, 0.0f, 1.0f, -3504.0f), vec4(0.0f, 0.0f, 1.0f, -3488.0f) },
		{ vec4(1.0f, 0.0f, 0.0f, -10.0f), vec4(0.0f, 1.0f, 0.0f, -10.0f) },
		{ vec4(0.0f, 0.0f, 1.0f, 0.0f), vec4(0.0f, 0.0f, -1.0f, 0.0f) },
	};
	for (const Case& c : cases)
	{
		Array<VisiblePortal> portals;
		portals.push_back(MakePortal(true, c.Held));
		CHECK_EQ(FindMirrorPortalToJoin(portals, c.Query), -1);
	}
}
