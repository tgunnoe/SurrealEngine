#pragma once

#include "Math/vec.h"
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

// One plane of a convex BSP leaf hull, stored as OverlapAABBModel reads it: (nx, ny, nz, w)
// with the plane at dot(n, p) = w. A plain plane keeps the hull on its negative side, so
// outward is +n; a flipped plane keeps it on the positive side, so outward is -n.
struct HullPlane
{
	dvec4 plane;
	bool flipped = false;
};

// The overlap test calls a box outside a plane only when it is strictly clear, so a box that
// merely touches a plane still overlaps. Exit distances carry this margin to end up clear.
constexpr double PlacementExitEpsilon = 0.01;

// The smallest move of an axis-aligned box along the outward direction of one hull plane that
// leaves the box outside that plane, or zero when it is already outside.
inline double PlaneExitDistance(const dvec3& center, const dvec3& extents, const HullPlane& p)
{
	double normalLength = std::sqrt(p.plane.x * p.plane.x + p.plane.y * p.plane.y + p.plane.z * p.plane.z);
	if (normalLength == 0.0)
		return 0.0; // a plane without a direction bounds nothing, so there is nothing to exit

	// The plane is not assumed to be unit length, so scale the signed distances to world units
	double halfExtent = (extents.x * std::abs(p.plane.x) + extents.y * std::abs(p.plane.y) + extents.z * std::abs(p.plane.z)) / normalLength;
	double centerSide = (center.x * p.plane.x + center.y * p.plane.y + center.z * p.plane.z - p.plane.w) / normalLength;

	double depth = p.flipped ? centerSide + halfExtent : halfExtent - centerSide;
	return depth < 0.0 ? 0.0 : depth + PlacementExitEpsilon;
}

// The outward direction (a unit vector) and distance of the shallowest face of the hull the box
// penetrates; the first of equally shallow faces wins. Nothing when the box is already outside
// some plane, because then it does not overlap the hull at all.
inline std::optional<std::pair<dvec3, double>> HullExit(const dvec3& center, const dvec3& extents, const std::vector<HullPlane>& hull)
{
	std::optional<std::pair<dvec3, double>> shallowest;
	for (const HullPlane& p : hull)
	{
		double distance = PlaneExitDistance(center, extents, p);
		if (distance == 0.0)
			return std::nullopt;

		if (!shallowest || distance < shallowest->second)
		{
			dvec3 normal = normalize(dvec3(p.plane.x, p.plane.y, p.plane.z));
			shallowest = std::make_pair(p.flipped ? -normal : normal, distance);
		}
	}
	return shallowest;
}

// The box center after moving out through the shallowest face; unchanged when there is no exit.
inline dvec3 ResolvedCenter(const dvec3& center, const dvec3& extents, const std::vector<HullPlane>& hull)
{
	std::optional<std::pair<dvec3, double>> exit = HullExit(center, extents, hull);
	return exit ? center + exit->first * exit->second : center;
}
