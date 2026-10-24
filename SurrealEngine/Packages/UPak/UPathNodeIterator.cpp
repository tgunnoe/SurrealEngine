
#include "Precomp.h"
#include "UPathNodeIterator.h"
#include "Packages/Engine/Actors/Info/ULevelInfo.h"
#include "Packages/Engine/Actors/NavigationPoint/UNavigationPoint.h"
#include "Packages/Engine/Actors/Pawn/UPawn_AIMath.h"
#include "Packages/Engine/Resources/Level/ULevel.h"
#include "Utils/Logger.h"
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <vector>

// The stock class declares NodePath as an array of 64, which is the store the seams fill.
static_assert(AIMath::MaxPathNodes == 64);

namespace
{
	// The index of the navigation point nearest a location by straight three-dimensional distance, the
	// first in list order when two are equally near, or -1 when there are none.
	int NearestNode(const std::vector<UNavigationPoint*>& navPoints, const vec3& location)
	{
		int nearest = -1;
		float nearestSquared = 0.0f;
		for (size_t node = 0; node < navPoints.size(); node++)
		{
			vec3 offset = navPoints[node]->Location() - location;
			float squared = dot(offset, offset);
			if (nearest < 0 || squared < nearestSquared)
			{
				nearest = (int)node;
				nearestSquared = squared;
			}
		}
		return nearest;
	}

	// Moves the iterator's cursor with one of the seam's cursor functions and answers the node it lands
	// on. A script can write NodeCount and NodeIndex, so the count is brought into the size of the store
	// here, where the field is read, and the function brings the index inside the path.
	UNavigationPoint* MoveCursor(UPakPathNodeIterator& iterator, std::optional<int> (*move)(AIMath::PathCursor&))
	{
		AIMath::PathCursor cursor{ std::clamp(iterator.NodeCount(), 0, AIMath::MaxPathNodes), iterator.NodeIndex() };
		std::optional<int> slot = move(cursor);
		iterator.NodeIndex() = cursor.index;
		return slot ? iterator.NodePath()[*slot] : nullptr;
	}
}

// BuildPath keeps the cheapest path from the navigation point nearest Start to the one at End, over the
// level's own reach specs. The semantics are designed from how the Predator calls the iterator, not
// recovered from the original: the graph has an edge for each spec a navigation point lists in Paths()
// that the level builder did not prune, costing the spec's distance, and no spec is dropped for the
// width, height or flags of a pawn, because the iterator has no pawn (the Predator tests reachability
// itself). End must lie within one unit of a navigation point, as Home.Location does, or there is no path.
// A failed call leaves an empty path, and NodeStart is Start either way.
void UPakPathNodeIterator::BuildPath(vec3& start, vec3& end)
{
	std::vector<UNavigationPoint*> navPoints;
	std::unordered_map<UNavigationPoint*, int> nodeOf;
	for (UNavigationPoint* navPoint = Level()->NavigationPointList(); navPoint; navPoint = navPoint->nextNavigationPoint())
	{
		nodeOf[navPoint] = (int)navPoints.size();
		navPoints.push_back(navPoint);
	}

	const Array<LevelReachSpec>& reachSpecs = XLevel()->ReachSpecs;
	AIMath::Graph graph(navPoints.size());
	for (size_t node = 0; node < navPoints.size(); node++)
	{
		for (int specIndex : navPoints[node]->Paths())
		{
			if (specIndex < 0 || (size_t)specIndex >= reachSpecs.size())
				break; // -1 ends the list

			const LevelReachSpec& reachSpec = reachSpecs[specIndex];
			auto to = nodeOf.find(reachSpec.endActor);
			if (reachSpec.bPruned != 0 || to == nodeOf.end())
				continue;

			graph[node].push_back({ to->second, reachSpec.distance, false });
		}
	}

	AIMath::PathStore store;
	int startNode = NearestNode(navPoints, start);
	int endNode = NearestNode(navPoints, end);
	if (startNode >= 0 && endNode >= 0)
	{
		vec3 gap = navPoints[endNode]->Location() - end;
		if (dot(gap, gap) <= 1.0f)
		{
			std::optional<AIMath::PathResult> path = AIMath::ShortestPath(graph, startNode, endNode, false);
			if (path)
				store = AIMath::StorePath(path->nodes, path->cost);
		}
	}

	auto nodePath = NodePath();
	for (int slot = 0; slot < AIMath::MaxPathNodes; slot++)
		nodePath[slot] = slot < store.count ? navPoints[store.nodes[slot]] : nullptr;
	NodeStart() = start;
	NodeCount() = store.count;
	NodeIndex() = 0;
	NodeCost() = store.cost;
}

UNavigationPoint* UPakPathNodeIterator::GetFirst()
{
	return MoveCursor(*this, AIMath::CursorFirst);
}

UNavigationPoint* UPakPathNodeIterator::GetPrevious()
{
	return MoveCursor(*this, AIMath::CursorPrevious);
}

UNavigationPoint* UPakPathNodeIterator::GetCurrent()
{
	return MoveCursor(*this, AIMath::CursorCurrent);
}

UNavigationPoint* UPakPathNodeIterator::GetNext()
{
	return MoveCursor(*this, AIMath::CursorNext);
}

UNavigationPoint* UPakPathNodeIterator::GetLast()
{
	return MoveCursor(*this, AIMath::CursorLast);
}

UNavigationPoint* UPakPathNodeIterator::GetLastVisible()
{
	LogUnimplemented("PathNodeIterator.GetLastVisible()");
	return nullptr;
}
