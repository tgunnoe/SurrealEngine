#pragma once

#include "Math/vec.h"
#include <vector>

// Arithmetic behind the Deus Ex AI natives, kept over plain values so it can be tested
// without a loaded package. Angles are Unreal rotator units: 65536 is one full turn.
namespace AIMath
{
	constexpr int AngleUnits = 65536;
	constexpr int Unreachable = -1;

	struct Edge
	{
		int to = 0;
		int cost = 0;
		bool pruned = false;
	};

	using Graph = std::vector<std::vector<Edge>>;

	inline int WrapSigned(int angle) { return 0; }
	inline float DegreesToUnits(float degrees) { return 0.0f; }
	inline bool InAngularWindow(int angle, int central, int halfWidth) { return false; }
	inline int SampleAngularWindow(int central, int halfWidth, float unit) { return 0; }
	inline vec3 Forward(int pitch, int yaw) { return vec3(0.0f); }
	inline vec3 PointAlong(const vec3& origin, int pitch, int yaw, float distance) { return origin; }
	inline bool InViewCone(const vec3& eye, int viewPitch, int viewYaw, int addPitch, int addYaw, const vec3& target, float peripheralCosine) { return false; }
	inline float SightMagnitude(float distance, float sightRadius, float visibility) { return 0.0f; }
	inline std::vector<float> RayDistances(float minDist, float maxDist, float step) { return {}; }
	inline std::vector<int> ShortestDistances(const Graph& graph, int start, bool usePruned) { return {}; }
}
