#pragma once

#include "Math/bbox.h"
#include "Math/mat.h"
#include "Math/vec.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// The grid cells a window covers: start is the first cell on each axis, end is one past the last.
// The cells are 256 units wide, the same grid as CollisionSystem, which keeps its cell helpers
// private; the formulas below restate GetStartExtents and GetEndExtents.
struct EncroachCellRange
{
	ivec3 start;
	ivec3 end;
};

// What the broad phase of a mover's encroachment test needs to know about the mover.
struct EncroachWindowSource
{
	BBox brushBox; // the brush's box in its own space
	vec3 location;
	mat4 rotation; // the mover's rotation as a matrix, object space to world space
	vec3 mainScale;
	vec3 prePivot;
	float radius = 0.0f;
	float height = 0.0f;
};

inline int EncroachCellOf(float coordinate)
{
	return (int)std::floor(coordinate * (1.0f / 256.0f));
}

// The cells whose actors are tested against the mover's brush.
inline EncroachCellRange EncroachWindow(const EncroachWindowSource& source)
{
	vec3 extents = { source.radius, source.radius, source.height };
	vec3 lo = source.location - extents;
	vec3 hi = source.location + extents;
	return {
		{ EncroachCellOf(lo.x), EncroachCellOf(lo.y), EncroachCellOf(lo.z) },
		{ EncroachCellOf(hi.x) + 1, EncroachCellOf(hi.y) + 1, EncroachCellOf(hi.z) + 1 } };
}

// One actor registered in a cell of the window. The identity is the caller's own handle for the actor,
// so an actor registered in several cells appears once per cell with the same identity.
template <typename Id>
struct EncroachCandidate
{
	Id id;
	bool isMover = false; // the moving brush itself
	bool isBrush = false; // any brush, which never counts as encroached upon
};

// The identities of the candidates that the mover encroaches on, each once, in the order first seen.
// The candidates arrive in cell order; overlaps decides whether one touches the mover's brush.
template <typename Id, typename Overlaps>
std::vector<Id> GatherEncroaching(const std::vector<EncroachCandidate<Id>>& candidates, Overlaps overlaps)
{
	std::vector<Id> hits;

	// The gather this replaces marks the mover's check counter rather than each candidate's, so
	// every record after the first is passed over.
	size_t examined = std::min<size_t>(candidates.size(), 1);
	for (size_t i = 0; i < examined; i++)
	{
		const EncroachCandidate<Id>& candidate = candidates[i];
		if (candidate.isMover || candidate.isBrush)
			continue;

		if (overlaps(candidate.id) && std::find(hits.begin(), hits.end(), candidate.id) == hits.end())
			hits.push_back(candidate.id);
	}
	return hits;
}
