#pragma once

#include "Math/vec.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

// Arithmetic behind the pawn AI natives and ticks, kept over plain values so it can be tested
// without a loaded package. Angles are Unreal rotator units: 65536 is one full turn.
namespace AIMath
{
	constexpr int AngleUnits = 65536;

	// Marks a node that no path reaches, in the distances ShortestDistances returns.
	constexpr int Unreachable = -1;

	// A ray is never sampled more often than this; a longer range is refused rather than walked.
	constexpr size_t MaxRaySamples = 4096;

	struct Edge
	{
		int to = 0;
		int cost = 0;
		// Set for connections the level builder dropped as redundant (reachable another way).
		bool pruned = false;
	};

	// Adjacency list indexed by node.
	using Graph = std::vector<std::vector<Edge>>;

	// Folds any angle into [-32768, 32767].
	inline int WrapSigned(int angle)
	{
		int wrapped = angle & (AngleUnits - 1);
		return wrapped > AngleUnits / 2 - 1 ? wrapped - AngleUnits : wrapped;
	}

	// True when angle is within halfWidth units of central, compared modulo a full turn.
	inline bool InAngularWindow(int angle, int central, int halfWidth)
	{
		return std::abs(WrapSigned(angle - central)) <= halfWidth;
	}

	// A script's yawDistribution and pitchDistribution are fractions of a half turn, not degrees: the
	// window either side of the central heading is distribution * 32768 units wide, up to the half turn
	// that makes it the whole circle. Every non-zero value the shipped scripts pass (0.25 as a bias toward
	// home, 0.5, 0.6 and 0.8 when fleeing) is such a fraction; as degrees they would be cones under one
	// degree wide, and as fractions they give plus or minus 45, 90, 108 and 144 degrees, which is what
	// those call sites want. The original mapping is not recoverable, so this is this engine's own rule.
	inline int HalfTurnFractionWidth(float fraction)
	{
		return std::min(AngleUnits / 2, (int)std::lround(std::min(fraction, 1.0f) * (AngleUnits / 2)));
	}

	// Half width of the yaw window for a script's yawDistribution. Scripts that want any direction pass a
	// central yaw of zero with a distribution of zero, so zero and below mean the whole circle rather than
	// a single heading.
	inline int YawHalfWidth(float distribution)
	{
		if (!(distribution > 0.0f))
			return AngleUnits / 2;
		return HalfTurnFractionWidth(distribution);
	}

	// Half width of the pitch window for pitchDistribution. Unlike yaw, zero and below mean exactly the
	// central pitch, which keeps a walking pawn's random picks level. It stops at a quarter turn: a pitch
	// past that points back over the pawn's shoulder (Forward folds over vertical), so the fleeing values
	// 0.6 and 0.8 would otherwise sample upward directions into open air.
	inline int PitchHalfWidth(float distribution)
	{
		if (!(distribution > 0.0f))
			return 0;
		return std::min(HalfTurnFractionWidth(distribution), AngleUnits / 4);
	}

	// Height for the centre of a walking pawn's random destination: one half height above the floor the
	// drop under the sampled point found, wherever that floor is (a ledge above the pawn is still a floor).
	// Empty when no floor lay within the drop, since a point with nothing to stand on is no destination for
	// a pawn that walks. hitZ means nothing without a hit.
	inline std::optional<float> GroundedZ(float hitZ, bool hasHit, float collisionHeight)
	{
		if (!hasHit)
			return std::nullopt;
		return hitZ + collisionHeight;
	}

	// Maps unit in [0, 1] linearly onto central +- halfWidth and wraps the result into [0, 65536).
	inline int SampleAngularWindow(int central, int halfWidth, float unit)
	{
		float clamped = std::clamp(unit, 0.0f, 1.0f);
		int offset = (int)std::lround((2.0f * clamped - 1.0f) * (float)halfWidth);
		return (central + offset) & (AngleUnits - 1);
	}

	// Unit facing vector for a pitch and yaw. Positive pitch looks up, positive yaw turns toward +Y.
	inline vec3 Forward(int pitch, int yaw)
	{
		const float toRadians = 3.14159265359f / (AngleUnits / 2);
		float p = WrapSigned(pitch) * toRadians;
		float y = WrapSigned(yaw) * toRadians;
		return vec3(std::cos(p) * std::cos(y), std::cos(p) * std::sin(y), std::sin(p));
	}

	inline vec3 PointAlong(const vec3& origin, int pitch, int yaw, float distance)
	{
		return origin + Forward(pitch, yaw) * distance;
	}

	// True when target lies inside the cone around the view direction whose half angle has the given
	// cosine (a pawn's PeripheralVision). The view is the base rotation plus the added rotation, the
	// way a pawn's AIAddViewRotation offsets where it looks. A target at the eye is always inside.
	inline bool InViewCone(const vec3& eye, int viewPitch, int viewYaw, int addPitch, int addYaw, const vec3& target, float peripheralCosine)
	{
		vec3 toTarget = target - eye;
		float distance = length(toTarget);
		if (distance <= FLT_EPSILON)
			return true;
		vec3 view = Forward(viewPitch + addPitch, viewYaw + addYaw);
		return dot(view, toTarget / distance) >= peripheralCosine;
	}

	// How strongly a target at distance is seen: visibility scales both the sight range and the value, and
	// the value falls off linearly to zero at the edge of the range. Always in [0, 1]; zero when unseen.
	inline float SightMagnitude(float distance, float sightRadius, float visibility)
	{
		if (!(visibility > 0.0f) || !(sightRadius > 0.0f))
			return 0.0f;
		float range = sightRadius * visibility;
		float clampedDistance = std::max(distance, 0.0f);
		if (!(clampedDistance < range))
			return 0.0f;
		return std::clamp(visibility * (1.0f - clampedDistance / range), 0.0f, 1.0f);
	}

	// Distances at which to sample a ray: minDist (not below zero), then every step, and finally maxDist
	// itself so a range shorter than one step is still tested at its far end. Strictly increasing.
	// Empty when the range is reversed, the step is not positive, or the range is non-finite or would
	// need more than MaxRaySamples samples.
	inline std::vector<float> RayDistances(float minDist, float maxDist, float step)
	{
		std::vector<float> distances;
		if (!(step > 0.0f) || !std::isfinite(minDist) || !std::isfinite(maxDist))
			return distances;
		float start = std::max(minDist, 0.0f);
		if (maxDist < start || (maxDist - start) / step > (float)MaxRaySamples)
			return distances;
		for (size_t i = 0; ; i++)
		{
			float distance = start + (float)i * step;
			if (distance >= maxDist)
				break;
			distances.push_back(distance);
		}
		distances.push_back(maxDist);
		return distances;
	}

	// Dijkstra from start. Entry n is the cost of the cheapest path to node n, or Unreachable. Edges
	// marked pruned are only followed when usePruned is set. A start outside the graph reaches nothing,
	// and an empty graph yields an empty result.
	inline std::vector<int> ShortestDistances(const Graph& graph, int start, bool usePruned)
	{
		std::vector<int> distance(graph.size(), Unreachable);
		if (start < 0 || (size_t)start >= graph.size())
			return distance;

		using Entry = std::pair<int, int>; // cost so far, node
		std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
		distance[start] = 0;
		queue.push({ 0, start });
		while (!queue.empty())
		{
			auto [cost, node] = queue.top();
			queue.pop();
			if (cost > distance[node])
				continue;
			for (const Edge& edge : graph[node])
			{
				if ((edge.pruned && !usePruned) || edge.to < 0 || (size_t)edge.to >= graph.size())
					continue;
				int candidate = cost + std::max(edge.cost, 0);
				int& best = distance[edge.to];
				if (best == Unreachable || candidate < best)
				{
					best = candidate;
					queue.push({ candidate, edge.to });
				}
			}
		}
		return distance;
	}

	// The body pitch UPawn::TickRotating writes for a desired pitch, given the pitch of the rotation rate.
	// The body stays within the rate of level on both sides, as the stock scripts keep a player's body: a
	// pitch inside that band is kept, one above it (up to the half turn) becomes the rate, and one below it
	// becomes the full turn minus the rate. The rate is brought into 0 to 32767 first, so the result is
	// always a pitch of 0 to 65535 and a rate of 0 holds the body level.
	inline int CapBodyPitch(int desiredPitch, int rate)
	{
		int pitch = desiredPitch & (AngleUnits - 1);
		int limit = std::clamp(rate, 0, AngleUnits / 2 - 1);
		if (pitch <= limit || pitch >= AngleUnits - limit)
			return pitch;
		if (pitch < AngleUnits / 2)
			return limit;
		return (AngleUnits - limit) & (AngleUnits - 1);
	}

	// The path node iterator of Unreal Gold's UPak.u (PathNodeIterator, which the Predator uses to flee)
	// is built on these functions. Its semantics are designed here, from the declaration of the stock class
	// and from what the Predator's calls need, and are not recovered from the original: a path is the
	// cheapest node sequence, kept in a store of at most MaxPathNodes nodes (the stock NodePath array),
	// and a cursor over the store answers no node before the first or after the last, so the loops the
	// Predator runs over it end.
	constexpr int MaxPathNodes = 64;

	struct PathResult
	{
		std::vector<int> nodes;
		int cost = 0;
	};

	// The cheapest path from start to end, both nodes of the graph, or nothing when end is unreachable or
	// either node is outside the graph. The nodes run from start to end and none repeats; a path from a node
	// to itself is that node alone at cost zero. Its cost equals the entry ShortestDistances gives for end.
	// Where two paths cost the same, the one the search settles first is returned.
	inline std::optional<PathResult> ShortestPath(const Graph& graph, int start, int end, bool usePruned)
	{
		int nodes = (int)graph.size();
		if (start < 0 || start >= nodes || end < 0 || end >= nodes)
			return std::nullopt;

		std::vector<int> distance(nodes, Unreachable);
		std::vector<int> previous(nodes, -1);

		using Entry = std::pair<int, int>; // cost so far, node
		std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
		distance[start] = 0;
		queue.push({ 0, start });
		while (!queue.empty())
		{
			auto [cost, node] = queue.top();
			queue.pop();
			if (cost > distance[node])
				continue;
			if (node == end)
				break;
			for (const Edge& edge : graph[node])
			{
				if ((edge.pruned && !usePruned) || edge.to < 0 || edge.to >= nodes)
					continue;
				int candidate = cost + std::max(edge.cost, 0);
				int& best = distance[edge.to];
				if (best == Unreachable || candidate < best)
				{
					best = candidate;
					previous[edge.to] = node;
					queue.push({ candidate, edge.to });
				}
			}
		}
		if (distance[end] == Unreachable)
			return std::nullopt;

		PathResult path;
		path.cost = distance[end];
		for (int node = end; node != -1; node = previous[node])
			path.nodes.push_back(node);
		std::reverse(path.nodes.begin(), path.nodes.end());
		return path;
	}

	// A path as the iterator keeps it: the first count nodes, in order, and the cost of the whole path.
	// Slots from count on are unspecified.
	struct PathStore
	{
		std::array<int, MaxPathNodes> nodes{};
		int count = 0;
		int cost = 0;
	};

	// A path longer than MaxPathNodes keeps its first MaxPathNodes nodes and its whole cost.
	inline PathStore StorePath(const std::vector<int>& nodes, int cost)
	{
		PathStore store;
		store.count = (int)std::min(nodes.size(), (size_t)MaxPathNodes);
		std::copy_n(nodes.begin(), store.count, store.nodes.begin());
		store.cost = cost;
		return store;
	}

	// The position of a walk over a store of count nodes (0 to MaxPathNodes). Each function below first
	// brings index into 0 to count-1 (0 when the path is empty), since a script can write it, then answers
	// the slot to read, or nothing when there is no such node. First, Last and Current answer nothing only
	// for an empty path; Next stays on the last node and Previous on the first when they run out.
	struct PathCursor
	{
		int count = 0;
		int index = 0;
	};

	// Brings the index inside the path; false when the path is empty.
	inline bool FitCursor(PathCursor& cursor)
	{
		if (cursor.count <= 0)
		{
			cursor.index = 0;
			return false;
		}
		cursor.index = std::clamp(cursor.index, 0, cursor.count - 1);
		return true;
	}

	inline std::optional<int> CursorFirst(PathCursor& cursor)
	{
		if (!FitCursor(cursor))
			return std::nullopt;
		cursor.index = 0;
		return cursor.index;
	}

	inline std::optional<int> CursorLast(PathCursor& cursor)
	{
		if (!FitCursor(cursor))
			return std::nullopt;
		cursor.index = cursor.count - 1;
		return cursor.index;
	}

	inline std::optional<int> CursorCurrent(PathCursor& cursor)
	{
		if (!FitCursor(cursor))
			return std::nullopt;
		return cursor.index;
	}

	inline std::optional<int> CursorNext(PathCursor& cursor)
	{
		if (!FitCursor(cursor) || cursor.index == cursor.count - 1)
			return std::nullopt;
		return ++cursor.index;
	}

	inline std::optional<int> CursorPrevious(PathCursor& cursor)
	{
		if (!FitCursor(cursor) || cursor.index == 0)
			return std::nullopt;
		return --cursor.index;
	}

	constexpr float LongFallSeconds = 0.0f;

	struct FallStep
	{
		float fallen = 0.0f;
		bool raise = false;
	};

	inline FallStep AdvanceFall(float fallen, float, float = LongFallSeconds)
	{
		return { fallen, false };
	}
}
