#pragma once

#include "Math/vec.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

// Arithmetic behind the Deus Ex AI natives, kept over plain values so it can be tested
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
	// drop under the sampled point found, wherever that floor is (a ledge above the pawn is still a floor),
	// or level with the pawn when no floor lay within the drop. hitZ means nothing without a hit.
	inline float GroundedZ(float hitZ, bool hasHit, float pawnZ, float collisionHeight)
	{
		return hasHit ? hitZ + collisionHeight : pawnZ;
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
}
