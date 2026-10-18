#include "Precomp.h"
#include "TestRunner.h"
#include "Math/coords.h"
#include "Math/rotator.h"
#include "Packages/Engine/Actors/Pawn/UPawn_AIMath.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <random>

// These cover the arithmetic behind the Deus Ex AI natives (sight, destination picking,
// reachable-node distances). The natives themselves need a loaded level and are checked by
// running maps, so only the plain-value functions are tested here.

namespace
{
	constexpr int Quarter = 16384; // 90 degrees in rotator units

	bool Near(float a, float b, float eps = 1e-4f)
	{
		return std::fabs(a - b) <= eps;
	}

	bool Near(const vec3& a, const vec3& b, float eps = 1e-4f)
	{
		return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
	}

	// Reference shortest paths by repeated relaxation, written independently of the Dijkstra under test.
	std::vector<int> BellmanFord(const AIMath::Graph& graph, int start, bool usePruned)
	{
		std::vector<int> dist(graph.size(), AIMath::Unreachable);
		if (start < 0 || start >= (int)graph.size())
			return dist;
		dist[start] = 0;
		for (size_t pass = 0; pass < graph.size(); pass++)
		{
			for (size_t from = 0; from < graph.size(); from++)
			{
				if (dist[from] == AIMath::Unreachable)
					continue;
				for (const AIMath::Edge& edge : graph[from])
				{
					if (edge.pruned && !usePruned)
						continue;
					int candidate = dist[from] + edge.cost;
					if (dist[edge.to] == AIMath::Unreachable || candidate < dist[edge.to])
						dist[edge.to] = candidate;
				}
			}
		}
		return dist;
	}

	// 0 -> 1 (10) -> 2 (10) -> 3 (5); 0 -> 2 (50) is the slow way; node 4 has no incoming edge.
	AIMath::Graph Chain()
	{
		AIMath::Graph graph(5);
		graph[0] = { { 1, 10, false }, { 2, 50, false } };
		graph[1] = { { 2, 10, false } };
		graph[2] = { { 3, 5, false } };
		return graph;
	}
}

TEST(AIAngleWrapSigned)
{
	CHECK_EQ(AIMath::WrapSigned(0), 0);
	CHECK_EQ(AIMath::WrapSigned(65536), 0);
	CHECK_EQ(AIMath::WrapSigned(40000), -25536);
	CHECK_EQ(AIMath::WrapSigned(-32769), 32767);
	CHECK_EQ(AIMath::WrapSigned(32767), 32767);
	CHECK_EQ(AIMath::WrapSigned(32768), -32768);
}

TEST(AIAngularWindowWrapsAroundFullTurn)
{
	// A window centred just below 65536 reaches across zero.
	CHECK(AIMath::InAngularWindow(65000, 65000, 1000));
	CHECK(AIMath::InAngularWindow(200, 65000, 1000));
	CHECK(AIMath::InAngularWindow(64600, 65000, 1000));
	CHECK(!AIMath::InAngularWindow(600, 65000, 1000));
	CHECK(!AIMath::InAngularWindow(30000, 65000, 1000));
	// Angles are compared modulo a full turn, so an unwrapped yaw still lands in the window.
	CHECK(AIMath::InAngularWindow(65536 + 200, 65000, 1000));
	CHECK(AIMath::InAngularWindow(-200, 65000, 1000));
}

TEST(AIAngularWindowHalfTurnOrWiderAcceptsEverything)
{
	for (int angle = 0; angle < AIMath::AngleUnits; angle += 4099)
		CHECK(AIMath::InAngularWindow(angle, 12345, 32768));
}

TEST(AIYawHalfWidthZeroMeansWholeCircle)
{
	// Scripts ask for "any direction" with a central yaw of 0 and a distribution of 0.
	CHECK_EQ(AIMath::YawHalfWidth(0.0f), 32768);
	CHECK_EQ(AIMath::YawHalfWidth(-3.0f), 32768);
	for (int angle = 0; angle < AIMath::AngleUnits; angle += 997)
		CHECK(AIMath::InAngularWindow(angle, 0, AIMath::YawHalfWidth(0.0f)));
}

TEST(AIYawHalfWidthIsFractionOfHalfTurn)
{
	CHECK_EQ(AIMath::YawHalfWidth(0.25f), 8192);
	CHECK_EQ(AIMath::YawHalfWidth(0.5f), 16384);
	CHECK_EQ(AIMath::YawHalfWidth(0.6f), 19661);
	CHECK_EQ(AIMath::YawHalfWidth(0.8f), 26214);
	CHECK_EQ(AIMath::YawHalfWidth(1.0f), 32768);
}

TEST(AIYawHalfWidthAboveOneIsClampedToHalfTurn)
{
	CHECK_EQ(AIMath::YawHalfWidth(1.5f), 32768);
	CHECK_EQ(AIMath::YawHalfWidth(720.0f), 32768);
}

TEST(AIHalfWidthFractionsMatchTheCallSites)
{
	// The values the shipped scripts pass, as the angle either side of the central heading:
	// 0.25 toward home, 0.5 and 0.6 and 0.8 when fleeing.
	const float unitsPerDegree = AIMath::AngleUnits / 360.0f;
	CHECK(Near((float)AIMath::YawHalfWidth(0.25f), 45.0f * unitsPerDegree, 1.0f));
	CHECK(Near((float)AIMath::YawHalfWidth(0.5f), 90.0f * unitsPerDegree, 1.0f));
	CHECK(Near((float)AIMath::YawHalfWidth(0.6f), 108.0f * unitsPerDegree, 1.0f));
	CHECK(Near((float)AIMath::YawHalfWidth(0.8f), 144.0f * unitsPerDegree, 1.0f));
}

TEST(AIPitchHalfWidthZeroMeansCentralPitchOnly)
{
	CHECK_EQ(AIMath::PitchHalfWidth(0.0f), 0);
	CHECK_EQ(AIMath::PitchHalfWidth(-5.0f), 0);
}

TEST(AIPitchHalfWidthStopsAtAQuarterTurn)
{
	CHECK_EQ(AIMath::PitchHalfWidth(0.25f), 8192);
	CHECK_EQ(AIMath::PitchHalfWidth(0.5f), Quarter);
	CHECK_EQ(AIMath::PitchHalfWidth(0.6f), Quarter);
	CHECK_EQ(AIMath::PitchHalfWidth(0.8f), Quarter);
	CHECK_EQ(AIMath::PitchHalfWidth(1.0f), Quarter);
	CHECK_EQ(AIMath::PitchHalfWidth(3.0f), Quarter);
}

TEST(AIPitchSampleNeverFoldsOverVertical)
{
	// A sample past a quarter turn points back over the pawn's shoulder, which is how a fleeing pawn
	// was handed destinations in the air. Whatever the distribution, a level central pitch must keep
	// every sampled direction on the near side of vertical.
	const float distributions[] = { 0.25f, 0.5f, 0.6f, 0.8f, 1.0f, 3.0f };
	for (float distribution : distributions)
	{
		int halfWidth = AIMath::PitchHalfWidth(distribution);
		for (int step = 0; step <= 200; step++)
		{
			int pitch = AIMath::SampleAngularWindow(0, halfWidth, step / 200.0f);
			CHECK_MSG(AIMath::Forward(pitch, 0).x >= -1e-4f,
				"distribution " + std::to_string(distribution) + " sampled pitch " + std::to_string(pitch) + " past vertical");
		}
	}
}

TEST(AIGroundedZStandsOnTheFloorBeneathTheSample)
{
	// The pawn's centre rests one half-height above the floor the drop found, wherever the sample was.
	std::optional<float> z = AIMath::GroundedZ(100.0f, true, 500.0f, 44.0f);
	CHECK(z.has_value());
	CHECK(Near(*z, 144.0f));
	z = AIMath::GroundedZ(-250.0f, true, 500.0f, 23.0f);
	CHECK(z.has_value());
	CHECK(Near(*z, -227.0f));
}

TEST(AIGroundedZWithNoFloorIsRefused)
{
	// A sample with nothing to stand on is no destination for a walking pawn, and the hit value that
	// goes with it must not leak into an answer.
	CHECK(!AIMath::GroundedZ(0.0f, false, 500.0f, 44.0f).has_value());
	CHECK(!AIMath::GroundedZ(9999.0f, false, -80.0f, 44.0f).has_value());
}

TEST(AIGroundedZFollowsAFloorAboveThePawn)
{
	// A ledge higher than the pawn is still a floor to stand on; the pawn's own height does not cap it.
	std::optional<float> z = AIMath::GroundedZ(600.0f, true, 500.0f, 44.0f);
	CHECK(z.has_value());
	CHECK(Near(*z, 644.0f));
}

TEST(AIAngularWindowSampleEndpoints)
{
	CHECK_EQ(AIMath::SampleAngularWindow(100, 300, 0.0f), 65336);
	CHECK_EQ(AIMath::SampleAngularWindow(100, 300, 0.5f), 100);
	CHECK_EQ(AIMath::SampleAngularWindow(100, 300, 1.0f), 400);
	CHECK_EQ(AIMath::SampleAngularWindow(65500, 100, 1.0f), 64);
	CHECK_EQ(AIMath::SampleAngularWindow(70000, 0, 0.3f), 70000 - 65536);
}

TEST(AIAngularWindowSampleStaysInsideWindow)
{
	std::mt19937 rng(1234);
	std::uniform_int_distribution<int> angle(-200000, 200000);
	std::uniform_int_distribution<int> half(0, 32767);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	for (int i = 0; i < 2000; i++)
	{
		int central = angle(rng);
		int halfWidth = half(rng);
		int sample = AIMath::SampleAngularWindow(central, halfWidth, unit(rng));
		CHECK_MSG(sample >= 0 && sample < AIMath::AngleUnits, "sample " + std::to_string(sample) + " outside [0, 65536)");
		CHECK_MSG(AIMath::InAngularWindow(sample, central, halfWidth),
			"sample " + std::to_string(sample) + " left window " + std::to_string(central) + " +- " + std::to_string(halfWidth));
	}
}

TEST(AIForwardAxes)
{
	CHECK(Near(AIMath::Forward(0, 0), vec3(1.0f, 0.0f, 0.0f)));
	CHECK(Near(AIMath::Forward(0, Quarter), vec3(0.0f, 1.0f, 0.0f)));
	CHECK(Near(AIMath::Forward(0, 2 * Quarter), vec3(-1.0f, 0.0f, 0.0f)));
	CHECK(Near(AIMath::Forward(Quarter, 0), vec3(0.0f, 0.0f, 1.0f)));
	CHECK(Near(AIMath::Forward(-Quarter, 0), vec3(0.0f, 0.0f, -1.0f)));
}

TEST(AIForwardMatchesEngineRotation)
{
	// The cone test must agree with how the engine turns a rotator into a facing direction.
	const int pitches[] = { 0, 1000, -3000, 8000, -12000 };
	const int yaws[] = { 0, 5461, 16384, 30000, 40000, 65000 };
	for (int pitch : pitches)
	{
		for (int yaw : yaws)
		{
			vec3 engineForward = Coords::Rotation(Rotator(pitch, yaw, 0)).XAxis;
			vec3 ours = AIMath::Forward(pitch, yaw);
			CHECK_MSG(Near(ours, engineForward, 1e-3f), "pitch " + std::to_string(pitch) + " yaw " + std::to_string(yaw));
		}
	}
}

TEST(AIPointAlongFollowsDirection)
{
	vec3 p = AIMath::PointAlong(vec3(10.0f, 20.0f, 30.0f), 0, Quarter, 100.0f);
	CHECK(Near(p, vec3(10.0f, 120.0f, 30.0f)));
	CHECK(Near(AIMath::PointAlong(vec3(1.0f, 2.0f, 3.0f), 0, 0, 0.0f), vec3(1.0f, 2.0f, 3.0f)));
}

TEST(AIViewConeInside)
{
	vec3 eye(0.0f, 0.0f, 0.0f);
	CHECK(AIMath::InViewCone(eye, 0, 0, 0, 0, vec3(100.0f, 0.0f, 0.0f), 0.5f));
	CHECK(AIMath::InViewCone(eye, 0, 0, 0, 0, vec3(100.0f, 30.0f, 0.0f), 0.5f));
}

TEST(AIViewConeOutside)
{
	vec3 eye(0.0f, 0.0f, 0.0f);
	CHECK(AIMath::InViewCone(eye, 0, 0, 0, 0, vec3(100.0f, 0.0f, 0.0f), 0.5f)); // same cone does admit dead ahead
	// Straight to the side is 90 degrees off, cosine 0, which is below a 0.5 limit.
	CHECK(!AIMath::InViewCone(eye, 0, 0, 0, 0, vec3(0.0f, 100.0f, 0.0f), 0.5f));
	CHECK(!AIMath::InViewCone(eye, 0, 0, 0, 0, vec3(100.0f, 200.0f, 0.0f), 0.5f));
}

TEST(AIViewConeBehind)
{
	vec3 eye(0.0f, 0.0f, 0.0f);
	vec3 behind(-100.0f, 0.0f, 0.0f);
	CHECK(!AIMath::InViewCone(eye, 0, 0, 0, 0, behind, 0.5f));
	CHECK(!AIMath::InViewCone(eye, 0, 0, 0, 0, behind, 0.0f));
	// A limit of -1 is the whole sphere: peripheral vision all the way round.
	CHECK(AIMath::InViewCone(eye, 0, 0, 0, 0, behind, -1.0f));
}

TEST(AIViewConeAddedRotationOffsetsView)
{
	vec3 eye(0.0f, 0.0f, 0.0f);
	vec3 ahead(100.0f, 0.0f, 0.0f);
	vec3 left(0.0f, 100.0f, 0.0f);
	vec3 above(0.0f, 0.0f, 100.0f);

	// Turning the view a quarter turn by yaw makes the +Y target visible and the +X one not.
	CHECK(AIMath::InViewCone(eye, 0, 0, 0, Quarter, left, 0.5f));
	CHECK(!AIMath::InViewCone(eye, 0, 0, 0, Quarter, ahead, 0.5f));

	// The added rotation sums with the view rotation, it does not replace it.
	CHECK(AIMath::InViewCone(eye, 0, Quarter, 0, -Quarter, ahead, 0.5f));
	CHECK(!AIMath::InViewCone(eye, 0, Quarter, 0, 0, ahead, 0.5f));

	// Pitch adds as well: looking a quarter turn up sees overhead.
	CHECK(AIMath::InViewCone(eye, 0, 0, Quarter, 0, above, 0.5f));
	CHECK(!AIMath::InViewCone(eye, 0, 0, 0, 0, above, 0.5f));
}

TEST(AIViewConeUsesEyeAsOrigin)
{
	vec3 eye(500.0f, -200.0f, 64.0f);
	CHECK(AIMath::InViewCone(eye, 0, 0, 0, 0, eye + vec3(80.0f, 0.0f, 0.0f), 0.5f));
	CHECK(!AIMath::InViewCone(eye, 0, 0, 0, 0, eye + vec3(-80.0f, 0.0f, 0.0f), 0.5f));
}

TEST(AIViewConeTargetAtEyeIsInside)
{
	vec3 eye(1.0f, 2.0f, 3.0f);
	CHECK(AIMath::InViewCone(eye, 0, 0, 0, 0, eye, 0.99f));
}

TEST(AISightMagnitudeFalloff)
{
	CHECK(Near(AIMath::SightMagnitude(0.0f, 1000.0f, 1.0f), 1.0f));
	CHECK(Near(AIMath::SightMagnitude(250.0f, 1000.0f, 1.0f), 0.75f));
	CHECK(Near(AIMath::SightMagnitude(500.0f, 1000.0f, 1.0f), 0.5f));
}

TEST(AISightMagnitudeZeroAtAndBeyondRange)
{
	CHECK(AIMath::SightMagnitude(999.0f, 1000.0f, 1.0f) > 0.0f);
	CHECK(Near(AIMath::SightMagnitude(1000.0f, 1000.0f, 1.0f), 0.0f));
	CHECK(Near(AIMath::SightMagnitude(1500.0f, 1000.0f, 1.0f), 0.0f));
	CHECK(Near(AIMath::SightMagnitude(600.0f, 1000.0f, 0.5f), 0.0f));
}

TEST(AISightMagnitudeScalesRangeAndValueByVisibility)
{
	// Half visibility halves the range and halves the value: 0.5 * (1 - 250 / 500).
	CHECK(Near(AIMath::SightMagnitude(250.0f, 1000.0f, 0.5f), 0.25f));
}

TEST(AISightMagnitudeDegenerateInputsSeeNothing)
{
	CHECK(AIMath::SightMagnitude(10.0f, 1000.0f, 1.0f) > 0.0f); // the same call with sane inputs sees
	CHECK(Near(AIMath::SightMagnitude(10.0f, 1000.0f, 0.0f), 0.0f));
	CHECK(Near(AIMath::SightMagnitude(10.0f, 1000.0f, -1.0f), 0.0f));
	CHECK(Near(AIMath::SightMagnitude(10.0f, 0.0f, 1.0f), 0.0f));
	CHECK(Near(AIMath::SightMagnitude(10.0f, -50.0f, 1.0f), 0.0f));
}

TEST(AISightMagnitudeIsBoundedAndNonIncreasing)
{
	for (float visibility : { 0.1f, 0.5f, 1.0f, 2.5f })
	{
		CHECK(AIMath::SightMagnitude(0.0f, 1000.0f, visibility) > 0.0f);
		float previous = 2.0f;
		for (float distance = 0.0f; distance <= 2500.0f; distance += 37.0f)
		{
			float value = AIMath::SightMagnitude(distance, 1000.0f, visibility);
			CHECK_MSG(value >= 0.0f && value <= 1.0f, "value " + std::to_string(value) + " outside [0, 1]");
			CHECK_MSG(value <= previous + 1e-6f, "value rose with distance at " + std::to_string(distance));
			previous = value;
		}
	}
}

TEST(AIRayDistancesStepOrder)
{
	std::vector<float> samples = AIMath::RayDistances(0.0f, 192.0f, 64.0f);
	CHECK_EQ(samples.size(), (size_t)4);
	CHECK(Near(samples[0], 0.0f));
	CHECK(Near(samples[1], 64.0f));
	CHECK(Near(samples[2], 128.0f));
	CHECK(Near(samples[3], 192.0f));
}

TEST(AIRayDistancesEndOnMaxWhenNotAMultiple)
{
	std::vector<float> samples = AIMath::RayDistances(40.0f, 200.0f, 64.0f);
	CHECK_EQ(samples.size(), (size_t)4);
	CHECK(Near(samples[0], 40.0f));
	CHECK(Near(samples[1], 104.0f));
	CHECK(Near(samples[2], 168.0f));
	CHECK(Near(samples[3], 200.0f));

	// A range shorter than one step still yields its far end, so a short seek is not dropped.
	std::vector<float> shortRange = AIMath::RayDistances(0.0f, 50.0f, 64.0f);
	CHECK_EQ(shortRange.size(), (size_t)2);
	CHECK(Near(shortRange[0], 0.0f));
	CHECK(Near(shortRange[1], 50.0f));
}

TEST(AIRayDistancesDegenerateRanges)
{
	std::vector<float> single = AIMath::RayDistances(100.0f, 100.0f, 64.0f);
	CHECK_EQ(single.size(), (size_t)1);
	CHECK(Near(single[0], 100.0f));
	CHECK(AIMath::RayDistances(200.0f, 100.0f, 64.0f).empty());
	CHECK(AIMath::RayDistances(0.0f, 100.0f, 0.0f).empty());
	CHECK(AIMath::RayDistances(0.0f, 100.0f, -8.0f).empty());
}

TEST(AIRayDistancesNegativeMinStartsAtZero)
{
	std::vector<float> samples = AIMath::RayDistances(-10.0f, 100.0f, 64.0f);
	CHECK_EQ(samples.size(), (size_t)3);
	CHECK(Near(samples[0], 0.0f));
	CHECK(Near(samples[1], 64.0f));
	CHECK(Near(samples[2], 100.0f));
}

TEST(AIRayDistancesRefusesRangesTooLongToWalk)
{
	// A range needing more samples than the cap is refused, not walked, so a runaway script value
	// cannot stall a tick on millions of traces.
	CHECK(AIMath::RayDistances(0.0f, 1000000.0f, 64.0f).empty());
	CHECK(!AIMath::RayDistances(0.0f, 4096.0f * 64.0f, 64.0f).empty());
}

TEST(AIRayDistancesRefusesNonFiniteRanges)
{
	const float inf = std::numeric_limits<float>::infinity();
	const float nan = std::numeric_limits<float>::quiet_NaN();
	CHECK(AIMath::RayDistances(0.0f, inf, 64.0f).empty());
	CHECK(AIMath::RayDistances(0.0f, nan, 64.0f).empty());
	CHECK(AIMath::RayDistances(nan, 100.0f, 64.0f).empty());
	CHECK(AIMath::RayDistances(0.0f, 100.0f, nan).empty());
}

TEST(AIRayDistancesAreStrictlyIncreasing)
{
	std::mt19937 rng(99);
	std::uniform_real_distribution<float> dist(0.0f, 600.0f);
	for (int i = 0; i < 500; i++)
	{
		float lo = dist(rng);
		float hi = lo + dist(rng);
		std::vector<float> samples = AIMath::RayDistances(lo, hi, 64.0f);
		CHECK(!samples.empty());
		CHECK(Near(samples.front(), lo));
		CHECK(Near(samples.back(), hi));
		for (size_t k = 1; k < samples.size(); k++)
			CHECK(samples[k] > samples[k - 1]);
	}
}

TEST(AIGraphShortestDistances)
{
	std::vector<int> dist = AIMath::ShortestDistances(Chain(), 0, false);
	CHECK_EQ(dist.size(), (size_t)5);
	CHECK_EQ(dist[0], 0);
	CHECK_EQ(dist[1], 10);
	CHECK_EQ(dist[2], 20); // via node 1, not the direct 50
	CHECK_EQ(dist[3], 25);
	CHECK_EQ(dist[4], AIMath::Unreachable);
}

TEST(AIGraphEdgesAreDirected)
{
	std::vector<int> dist = AIMath::ShortestDistances(Chain(), 3, false);
	CHECK_EQ(dist[3], 0);
	CHECK_EQ(dist[0], AIMath::Unreachable);
	CHECK_EQ(dist[1], AIMath::Unreachable);
	CHECK_EQ(dist[2], AIMath::Unreachable);
}

TEST(AIGraphPrunedEdgeChangesDistanceWhenHonoured)
{
	AIMath::Graph graph = Chain();
	graph[0].push_back({ 3, 12, true }); // a pruned shortcut from 0 to 3

	std::vector<int> withoutPruned = AIMath::ShortestDistances(graph, 0, false);
	CHECK_EQ(withoutPruned[3], 25);

	std::vector<int> withPruned = AIMath::ShortestDistances(graph, 0, true);
	CHECK_EQ(withPruned[3], 12);
	// Nodes not reached through the pruned edge keep their distance.
	CHECK_EQ(withPruned[1], 10);
	CHECK_EQ(withPruned[2], 20);
}

TEST(AIGraphPrunedEdgeCanReachOtherwiseIsolatedNode)
{
	AIMath::Graph graph = Chain();
	graph[3].push_back({ 4, 7, true });
	CHECK_EQ(AIMath::ShortestDistances(graph, 0, false)[4], AIMath::Unreachable);
	CHECK_EQ(AIMath::ShortestDistances(graph, 0, true)[4], 32);
}

TEST(AIGraphEmptyGraph)
{
	CHECK(AIMath::ShortestDistances(AIMath::Graph(), 0, false).empty());
	CHECK(AIMath::ShortestDistances(AIMath::Graph(), 0, true).empty());
}

TEST(AIGraphStartOutOfRangeReachesNothing)
{
	for (int start : { -1, 5, 100 })
	{
		std::vector<int> dist = AIMath::ShortestDistances(Chain(), start, true);
		CHECK_EQ(dist.size(), (size_t)5);
		for (int d : dist)
			CHECK_EQ(d, AIMath::Unreachable);
	}
}

TEST(AIGraphZeroCostEdgesAndCycles)
{
	AIMath::Graph graph(3);
	graph[0] = { { 1, 0, false } };
	graph[1] = { { 2, 4, false }, { 0, 0, false } }; // cycle back to the start
	graph[2] = { { 0, 1, false } };
	std::vector<int> dist = AIMath::ShortestDistances(graph, 0, false);
	CHECK_EQ(dist[0], 0);
	CHECK_EQ(dist[1], 0);
	CHECK_EQ(dist[2], 4);
}

TEST(AIGraphMatchesRelaxationReferenceOnRandomGraphs)
{
	std::mt19937 rng(20251018);
	for (int round = 0; round < 300; round++)
	{
		int nodes = 1 + (int)(rng() % 12);
		AIMath::Graph graph(nodes);
		for (int from = 0; from < nodes; from++)
		{
			for (int to = 0; to < nodes; to++)
			{
				if (rng() % 100 < 22)
					graph[from].push_back({ to, (int)(rng() % 90), rng() % 4 == 0 });
			}
		}
		int start = (int)(rng() % nodes);
		for (bool usePruned : { false, true })
		{
			std::vector<int> expected = BellmanFord(graph, start, usePruned);
			std::vector<int> actual = AIMath::ShortestDistances(graph, start, usePruned);
			CHECK_MSG(actual == expected, "round " + std::to_string(round) + " start " + std::to_string(start) + " usePruned " + std::to_string(usePruned));
		}
	}
}

TEST(AIGraphAllowingPrunedEdgesNeverLengthensAPath)
{
	std::mt19937 rng(7);
	for (int round = 0; round < 200; round++)
	{
		int nodes = 2 + (int)(rng() % 10);
		AIMath::Graph graph(nodes);
		for (int from = 0; from < nodes; from++)
		{
			for (int to = 0; to < nodes; to++)
			{
				if (rng() % 100 < 25)
					graph[from].push_back({ to, 1 + (int)(rng() % 60), rng() % 3 == 0 });
			}
		}
		std::vector<int> strict = AIMath::ShortestDistances(graph, 0, false);
		std::vector<int> loose = AIMath::ShortestDistances(graph, 0, true);
		CHECK_EQ(strict.size(), (size_t)nodes);
		CHECK_EQ(loose.size(), (size_t)nodes);
		for (int n = 0; n < nodes; n++)
		{
			if (strict[n] != AIMath::Unreachable)
				CHECK(loose[n] != AIMath::Unreachable && loose[n] <= strict[n]);
		}
	}
}
