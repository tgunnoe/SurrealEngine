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

TEST(MirrorMatrixReflectsYAndZNormals)
{
	CheckMirrorMatrixReflects(vec3(0.0f, 1.0f, 0.0f));
	CheckMirrorMatrixReflects(vec3(0.0f, -1.0f, 0.0f));
	CheckMirrorMatrixReflects(vec3(0.0f, 0.0f, 1.0f));
	CheckMirrorMatrixReflects(vec3(0.0f, 0.0f, -1.0f));
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
