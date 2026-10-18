#pragma once

#include "Math/vec.h"
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

struct HullPlane
{
	dvec4 plane;
	bool flipped = false;
};

constexpr double PlacementExitEpsilon = 0.01;

inline double PlaneExitDistance(const dvec3& center, const dvec3& extents, const HullPlane& p)
{
	return -1.0;
}

inline std::optional<std::pair<dvec3, double>> HullExit(const dvec3& center, const dvec3& extents, const std::vector<HullPlane>& hull)
{
	return std::make_pair(dvec3(0.0), -1.0);
}

inline dvec3 ResolvedCenter(const dvec3& center, const dvec3& extents, const std::vector<HullPlane>& hull)
{
	return dvec3(0.0);
}
