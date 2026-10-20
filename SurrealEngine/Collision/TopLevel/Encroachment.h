#pragma once

#include "Math/bbox.h"
#include "Math/mat.h"
#include "Math/vec.h"
#include <cmath>
#include <set>
#include <vector>

// The grid cells a window covers: start is the first cell on each axis, end is one past the last.
// The cells are 256 units wide, the same grid as CollisionSystem, which keeps its cell helpers
// private; the formulas below restate GetStartExtents and GetEndExtents.
struct EncroachCellRange
{
	ivec3 start;
	ivec3 end;
};

// Where a mover's brush stands, as the broad phase of its encroachment test needs it.
struct EncroachWindowSource
{
	BBox brushBox; // the brush's box in its own space
	vec3 location;
	mat4 rotation; // the mover's rotation as a matrix, object space to world space
	vec3 mainScale;
	vec3 prePivot;
};

inline int EncroachCellOf(float coordinate)
{
	return (int)std::floor(coordinate * (1.0f / 256.0f));
}

// The cells whose actors are tested against the mover's brush: those the brush's world box covers,
// placed the way the engine places that box when it draws and registers the mover.
inline EncroachCellRange EncroachWindow(const EncroachWindowSource& source)
{
	mat4 objectToWorld = mat4::translate(source.location) * source.rotation * mat4::scale(source.mainScale) * mat4::translate(-source.prePivot);
	BBox box = source.brushBox.transform(objectToWorld);
	return {
		{ EncroachCellOf(box.min.x), EncroachCellOf(box.min.y), EncroachCellOf(box.min.z) },
		{ EncroachCellOf(box.max.x) + 1, EncroachCellOf(box.max.y) + 1, EncroachCellOf(box.max.z) + 1 } };
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
// The candidates arrive in cell order; overlaps decides whether one touches the mover's brush, and
// is asked once per identity however many cells list it.
template <typename Id, typename Overlaps>
std::vector<Id> GatherEncroaching(const std::vector<EncroachCandidate<Id>>& candidates, Overlaps overlaps)
{
	std::vector<Id> hits;
	std::set<Id> examined;
	for (const EncroachCandidate<Id>& candidate : candidates)
	{
		if (candidate.isMover || candidate.isBrush)
			continue;

		if (examined.insert(candidate.id).second && overlaps(candidate.id))
			hits.push_back(candidate.id);
	}
	return hits;
}
