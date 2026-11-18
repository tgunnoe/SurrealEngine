#include "Precomp.h"
#include "TestRunner.h"
#include "Collision/TopLevel/Placement.h"
#include <random>

// The placement resolver moves a box that penetrates a convex BSP leaf hull out through the
// hull face it is shallowest in. The planes follow OverlapAABBModel: a non-flipped plane has
// the hull on its negative side, a flipped one on its positive side, and a box that only
// touches a plane still overlaps (it is outside only when strictly clear of it).

namespace
{
	const double Tolerance = 1e-9;

	bool Near(double a, double b)
	{
		return std::abs(a - b) <= Tolerance;
	}

	bool Near(const dvec3& a, const dvec3& b)
	{
		return Near(a.x, b.x) && Near(a.y, b.y) && Near(a.z, b.z);
	}

	dvec3 Normal(const HullPlane& p)
	{
		return dvec3(p.plane.x, p.plane.y, p.plane.z);
	}

	// Independent of the resolver's projected-half-extent formula: the box is outside a plane
	// when each of its eight corners lies strictly on the outward side.
	bool BoxOutsidePlane(const dvec3& center, const dvec3& extents, const HullPlane& p)
	{
		for (int i = 0; i < 8; i++)
		{
			dvec3 corner = center + dvec3((i & 1) ? extents.x : -extents.x, (i & 2) ? extents.y : -extents.y, (i & 4) ? extents.z : -extents.z);
			double side = dot(Normal(p), corner) - p.plane.w;
			if (p.flipped ? side >= 0.0 : side <= 0.0)
				return false;
		}
		return true;
	}

	bool BoxOutsideAnyPlane(const dvec3& center, const dvec3& extents, const std::vector<HullPlane>& hull)
	{
		for (const HullPlane& p : hull)
		{
			if (BoxOutsidePlane(center, extents, p))
				return true;
		}
		return false;
	}

	// The cube [0, 100] on every axis, as six outward-facing planes in the order +x, -x, +y, -y, +z, -z
	std::vector<HullPlane> RoomHull()
	{
		return {
			{ dvec4(1.0, 0.0, 0.0, 100.0), false }, { dvec4(-1.0, 0.0, 0.0, 0.0), false },
			{ dvec4(0.0, 1.0, 0.0, 100.0), false }, { dvec4(0.0, -1.0, 0.0, 0.0), false },
			{ dvec4(0.0, 0.0, 1.0, 100.0), false }, { dvec4(0.0, 0.0, -1.0, 0.0), false } };
	}
}

TEST(PlacementRoomExitsThroughNearestFace)
{
	const dvec3 extents(10.0, 10.0, 10.0);
	const dvec3 center(50.0, 50.0, 20.0); // 30 deep in the floor face, 60 or more in every other
	std::vector<HullPlane> room = RoomHull();

	auto exit = HullExit(center, extents, room);
	CHECK(exit.has_value());
	CHECK(Near(exit->first, dvec3(0.0, 0.0, -1.0)));
	CHECK(Near(exit->second, 30.0 + PlacementExitEpsilon));

	dvec3 resolved = ResolvedCenter(center, extents, room);
	CHECK(Near(resolved, dvec3(50.0, 50.0, 20.0 - 30.0 - PlacementExitEpsilon)));
	CHECK(BoxOutsidePlane(resolved, extents, room[5]));
}

TEST(PlacementFloorPenetrationExitsUpward)
{
	// A solid hull under a floor at z=0, wide and deep enough that only the floor face is shallow
	std::vector<HullPlane> hull = {
		{ dvec4(0.0, 0.0, 1.0, 0.0), false }, { dvec4(0.0, 0.0, -1.0, 1000.0), false },
		{ dvec4(1.0, 0.0, 0.0, 1000.0), false }, { dvec4(-1.0, 0.0, 0.0, 1000.0), false },
		{ dvec4(0.0, 1.0, 0.0, 1000.0), false }, { dvec4(0.0, -1.0, 0.0, 1000.0), false } };
	const dvec3 extents(20.0, 20.0, 47.5);
	const dvec3 center(0.0, 0.0, 40.1); // the box bottom is 7.4 below the floor

	auto exit = HullExit(center, extents, hull);
	CHECK(exit.has_value());
	CHECK(Near(exit->first, dvec3(0.0, 0.0, 1.0)));
	CHECK(Near(exit->second, 7.4 + PlacementExitEpsilon));

	dvec3 resolved = ResolvedCenter(center, extents, hull);
	CHECK(Near(resolved.z, 40.1 + 7.4 + PlacementExitEpsilon));
	CHECK(BoxOutsidePlane(resolved, extents, hull[0]));
}

TEST(PlacementFlippedPlaneExitsAlongNegativeNormal)
{
	// The hull is the half space above z=0, so the way out is down
	std::vector<HullPlane> hull = { { dvec4(0.0, 0.0, 1.0, 0.0), true } };
	const dvec3 extents(10.0, 10.0, 10.0);
	const dvec3 center(0.0, 0.0, 5.0);

	auto exit = HullExit(center, extents, hull);
	CHECK(exit.has_value());
	CHECK(Near(exit->first, dvec3(0.0, 0.0, -1.0)));
	CHECK(Near(exit->second, 15.0 + PlacementExitEpsilon));

	dvec3 resolved = ResolvedCenter(center, extents, hull);
	CHECK(Near(resolved.z, 5.0 - 15.0 - PlacementExitEpsilon));
	CHECK(BoxOutsidePlane(resolved, extents, hull[0]));
}

TEST(PlacementBoxOutsideAnyPlaneYieldsNoExit)
{
	const dvec3 extents(10.0, 10.0, 10.0);
	const dvec3 center(50.0, 50.0, 200.0); // clear of the roof face of the room
	std::vector<HullPlane> room = RoomHull();

	CHECK_EQ(PlaneExitDistance(center, extents, room[4]), 0.0);
	CHECK(!HullExit(center, extents, room).has_value());
	CHECK(Near(ResolvedCenter(center, extents, room), center));

	HullPlane below = { dvec4(0.0, 0.0, 1.0, 0.0), true };
	CHECK_EQ(PlaneExitDistance(dvec3(0.0, 0.0, -50.0), extents, below), 0.0);
}

TEST(PlacementTouchingFaceStillCountsAsInside)
{
	// The overlap test calls a box outside only when strictly clear, so touching needs the margin
	HullPlane floor = { dvec4(0.0, 0.0, 1.0, 0.0), false };
	const dvec3 extents(20.0, 20.0, 47.5);

	CHECK(Near(PlaneExitDistance(dvec3(0.0, 0.0, 47.5), extents, floor), PlacementExitEpsilon));
}

TEST(PlacementSlopedPlaneExitsAlongItsNormal)
{
	HullPlane slope = { dvec4(0.664, 0.0, -0.747, 0.0), false };
	const dvec3 normal = normalize(dvec3(0.664, 0.0, -0.747));
	const dvec3 extents(20.0, 20.0, 47.5);
	const dvec3 center(0.0, 0.0, 0.0);

	// The projection of the box onto the unit normal, plus the margin
	double projected = extents.x * std::abs(normal.x) + extents.y * std::abs(normal.y) + extents.z * std::abs(normal.z);

	auto exit = HullExit(center, extents, { slope });
	CHECK(exit.has_value());
	CHECK(Near(exit->first, normal));
	CHECK(Near(exit->second, projected + PlacementExitEpsilon));

	dvec3 resolved = ResolvedCenter(center, extents, { slope });
	CHECK(Near(resolved, normal * (projected + PlacementExitEpsilon)));
	CHECK(BoxOutsidePlane(resolved, extents, slope));
}

TEST(PlacementTieBetweenFacesPicksTheFirst)
{
	// Both faces of the slab are 60 deep, so the plane listed first wins
	HullPlane right = { dvec4(1.0, 0.0, 0.0, 100.0), false };
	HullPlane left = { dvec4(-1.0, 0.0, 0.0, 0.0), false };
	const dvec3 extents(10.0, 10.0, 10.0);
	const dvec3 center(50.0, 0.0, 0.0);

	auto rightFirst = HullExit(center, extents, { right, left });
	CHECK(rightFirst.has_value());
	CHECK(Near(rightFirst->first, dvec3(1.0, 0.0, 0.0)));

	auto leftFirst = HullExit(center, extents, { left, right });
	CHECK(leftFirst.has_value());
	CHECK(Near(leftFirst->first, dvec3(-1.0, 0.0, 0.0)));
}

TEST(PlacementExitMatchesCornerOracleOnRandomHulls)
{
	std::mt19937 rng(20260101);
	std::uniform_real_distribution<double> coord(-1.0, 1.0);

	for (int iteration = 0; iteration < 2000; iteration++)
	{
		std::vector<HullPlane> hull;
		int planeCount = 4 + (int)(rng() % 5);
		while ((int)hull.size() < planeCount)
		{
			dvec3 n(coord(rng), coord(rng), coord(rng));
			if (length(n) < 0.2)
				continue;
			hull.push_back({ dvec4(n.x, n.y, n.z, 100.0 * coord(rng)), (rng() & 1) != 0 });
		}
		const dvec3 extents(1.0 + 49.0 * std::abs(coord(rng)), 1.0 + 49.0 * std::abs(coord(rng)), 1.0 + 49.0 * std::abs(coord(rng)));
		const dvec3 center(100.0 * coord(rng), 100.0 * coord(rng), 100.0 * coord(rng));
		std::string where = "iteration " + std::to_string(iteration);

		// Each plane's exit distance equals the smallest move along its outward direction that
		// the corner test calls clear, found by bisection
		for (const HullPlane& p : hull)
		{
			dvec3 outward = p.flipped ? -normalize(Normal(p)) : normalize(Normal(p));
			double lo = 0.0, hi = 2000.0;
			CHECK_MSG(BoxOutsidePlane(center + outward * hi, extents, p), where + ": bisection bound too small");
			if (BoxOutsidePlane(center, extents, p))
			{
				CHECK_MSG(PlaneExitDistance(center, extents, p) == 0.0, where + ": clear box reported a nonzero exit");
				continue;
			}
			for (int step = 0; step < 60; step++)
			{
				double mid = 0.5 * (lo + hi);
				(BoxOutsidePlane(center + outward * mid, extents, p) ? hi : lo) = mid;
			}
			CHECK_MSG(std::abs(PlaneExitDistance(center, extents, p) - PlacementExitEpsilon - hi) < 1e-6, where + ": exit distance differs from the corner oracle");
		}

		auto exit = HullExit(center, extents, hull);
		if (BoxOutsideAnyPlane(center, extents, hull))
		{
			CHECK_MSG(!exit.has_value(), where + ": exit reported for a box outside the hull");
			CHECK_MSG(Near(ResolvedCenter(center, extents, hull), center), where + ": clear box was moved");
			continue;
		}

		CHECK_MSG(exit.has_value(), where + ": no exit for a penetrating box");
		CHECK_MSG(Near(length(exit->first), 1.0), where + ": exit direction is not a unit vector");
		CHECK_MSG(exit->second >= PlacementExitEpsilon, where + ": exit distance is below the margin");
		for (const HullPlane& p : hull)
			CHECK_MSG(exit->second <= PlaneExitDistance(center, extents, p) + Tolerance, where + ": a shallower face exists");
		CHECK_MSG(BoxOutsideAnyPlane(ResolvedCenter(center, extents, hull), extents, hull), where + ": resolved box is still inside every face");
	}
}
