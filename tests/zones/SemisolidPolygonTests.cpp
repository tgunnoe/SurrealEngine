#include "Precomp.h"
#include "TestRunner.h"
#include "Collision/BottomLevel/TraceAABBModel.h"
#include "Packages/Engine/Resources/Level/UModel.h"
#include <cmath>
#include <cstring>
#include <iterator>

// Box sweeps against semisolid level geometry, on level models built by hand.
//
// The box trace reads leaf hulls and nothing else, so a semisolid polygon stops a
// sweeping box only where a hull carries its plane. These tests pin what the trace
// reports today, so that any change to how it treats semisolid polygons is measured
// against them.

namespace
{
	constexpr int Flip = 0x4000'0000;
	constexpr int NoLink = -1;

	const dvec3 Down(0.0, 0.0, -1.0);
	const dvec3 PlayerExtents(17.0, 17.0, 39.0);

	// A node with every link and the hull offset set to "none"; an aggregate's zero would
	// be a link to node 0 and a hull at the start of LeafHulls.
	BspNode MakeNode(float x, float y, float z, float w)
	{
		BspNode node{};
		node.PlaneX = x;
		node.PlaneY = y;
		node.PlaneZ = z;
		node.PlaneW = w;
		node.Front = NoLink;
		node.Back = NoLink;
		node.Plane = NoLink;
		node.CollisionBound = NoLink;
		node.Surf = NoLink;
		node.NumVertices = 0;
		return node;
	}

	int FloatBits(float value)
	{
		int bits;
		static_assert(sizeof(bits) == sizeof(value));
		std::memcpy(&bits, &value, sizeof(bits));
		return bits;
	}

	// A hull in the layout the trace reads: the node indices, a negative terminator and
	// the six floats of the bounding box (minimum, then maximum), stored as int32 bit patterns.
	void AppendHull(Array<int32_t>& leafHulls, const std::vector<int>& entries, const float (&bbox)[6])
	{
		for (int entry : entries)
			leafHulls.push_back(entry);
		leafHulls.push_back(-1);
		for (float value : bbox)
			leafHulls.push_back(FloatBits(value));
	}

	CollisionHitList SweepDown(UModel* model, double x, double y, double z)
	{
		TraceAABBModel tracer;
		return tracer.Trace(model, dvec3(x, y, z), 0.0, Down, 100.0, PlayerExtents, false);
	}
}

// A semisolid polygon that has a hull is one hit, and a later change that tests such
// polygons directly must not report it a second time.
TEST(HulledSemisolidReportsOneHit)
{
	UModel* model = GC::Alloc<UModel>("ZonesHulledSemisolid", nullptr, ObjectFlags::NoFlags);

	// Node 0 is the polygon: a 128 by 128 square facing up at z 384, with a hull.
	// Nodes 1 to 5 hold the slab's other five planes and are reachable only through the hull.
	BspNode polygon = MakeNode(0.0f, 0.0f, 1.0f, 384.0f);
	polygon.CollisionBound = 0;
	polygon.NumVertices = 4;
	polygon.VertPool = 0;
	polygon.Surf = 0;
	model->Nodes.push_back(polygon);
	model->Nodes.push_back(MakeNode(0.0f, 0.0f, -1.0f, -376.0f));
	model->Nodes.push_back(MakeNode(1.0f, 0.0f, 0.0f, 64.0f));
	model->Nodes.push_back(MakeNode(-1.0f, 0.0f, 0.0f, 64.0f));
	model->Nodes.push_back(MakeNode(0.0f, 1.0f, 0.0f, 64.0f));
	model->Nodes.push_back(MakeNode(0.0f, -1.0f, 0.0f, 64.0f));

	model->Points.push_back(vec3(-64.0f, -64.0f, 384.0f));
	model->Points.push_back(vec3(64.0f, -64.0f, 384.0f));
	model->Points.push_back(vec3(64.0f, 64.0f, 384.0f));
	model->Points.push_back(vec3(-64.0f, 64.0f, 384.0f));
	for (int i = 0; i < 4; i++)
		model->Vertices.push_back(BspVert{ i, 0 });

	BspSurface surface{};
	surface.PolyFlags = PF_Semisolid;
	model->Surfaces.push_back(surface);

	AppendHull(model->LeafHulls, { 0, 1, 2, 3, 4, 5 }, { -64.0f, -64.0f, 376.0f, 64.0f, 64.0f, 384.0f });

	// The box's bottom, 39 below its centre, reaches z 384 after 21 units; the hull path
	// takes 0.1 off the hit fraction.
	CollisionHitList hits = SweepDown(model, 0.0, 0.0, 444.0);
	CHECK_EQ(std::distance(hits.begin(), hits.end()), 1);
	CHECK(std::abs(hits.front().Fraction - 21.0f) <= 0.3f);
	CHECK(hits.front().Normal.z > 0.9f);
}

// The planes of the hulls that meet a player's column over the perimeter of a stock
// ring-shaped slab (Unreal Tournament's CTF-Nucleus, a semisolid brush whose top face
// is at z 384), as the level model stores them: the eleven leaf hulls whose bounding
// boxes meet a 17 unit radius column at (28, 3516) between z 245 and z 483, and the
// planes they list. The hulls are linked here through a chain of nodes whose own
// planes the sweep never straddles, so that only the hulls decide the result.
TEST(PerimeterHullTransplant)
{
	struct PlaneRow { float x, y, z, w; };

	// The distinct planes the eleven hulls reference, in the order of first use.
	const PlaneRow Planes[] = {
		{ 1.22460635e-16f, 1.0f, 0.0f, 3543.99902f }, // source node 7151
		{ 0.0f, 0.0f, -1.0f, -377.0f }, // source node 7343
		{ 0.86602664f, 0.499997824f, 2.98021927e-08f, 1790.13574f }, // source node 7476
		{ -1.0f, 1.22460635e-16f, 0.0f, -24.0008545f }, // source node 7526
		{ -0.866027176f, -0.499996901f, 0.0f, -1782.13245f }, // source node 7529
		{ -0.0f, 0.0f, -1.0f, -376.0f }, // source node 7585
		{ 1.0f, -1.22460635e-16f, 0.0f, 32.0008545f }, // source node 7471
		{ -0.866027176f, 0.49999693f, 2.98021394e-08f, 1729.84399f }, // source node 7520
		{ -0.0f, 0.0f, -1.0f, -376.0f }, // source node 7582
		{ 0.382683575f, 0.923879087f, 0.0f, 3238.53931f }, // source node 3185
		{ 0.86602664f, -0.499997824f, 0.0f, -1721.84705f }, // source node 7462
		{ -0.0f, 0.0f, -1.0f, -376.0f }, // source node 7579
		{ 0.0f, 0.0f, 1.0f, 383.0f }, // source node 7353
		{ 0.86602664f, 0.499997824f, 2.98021927e-08f, 1790.13574f }, // source node 7477
		{ -1.0f, 1.22460635e-16f, 0.0f, -24.0008545f }, // source node 7527
		{ -0.866027176f, -0.499996901f, 0.0f, -1782.13245f }, // source node 7530
		{ 0.0f, 0.0f, 1.0f, 384.0f }, // source node 7624
		{ 1.0f, -1.22460635e-16f, 0.0f, 32.0008545f }, // source node 7472
		{ -0.866027176f, 0.49999693f, 2.98021394e-08f, 1729.84399f }, // source node 7521
		{ 0.0f, 0.0f, 1.0f, 384.0f }, // source node 7621
		{ 0.86602664f, -0.499997824f, 0.0f, -1721.84705f }, // source node 7463
		{ 0.0f, 0.0f, 1.0f, 384.0f }, // source node 7618
		{ 0.841520965f, -0.540224493f, 0.0f, -1896.50842f }, // source node 7363
		{ 0.86602664f, 0.499997824f, 2.98021927e-08f, 1790.13574f }, // source node 7478
		{ -1.0f, 1.22460635e-16f, 0.0f, -24.0008545f }, // source node 7528
		{ -0.866027176f, -0.499996901f, 0.0f, -1782.13245f }, // source node 7531
		{ 1.0f, -1.22460635e-16f, 0.0f, 32.0008545f }, // source node 7473
		{ -0.866027176f, 0.49999693f, 2.98021394e-08f, 1729.84399f }, // source node 7522
		{ 0.86602664f, -0.499997824f, 0.0f, -1721.84705f }, // source node 7464
		{ -1.22460635e-16f, -1.0f, -0.0f, -1868.0f }, // source node 0
		{ 0.831469595f, -0.555570245f, -0.0f, -1964.46448f }, // source node 2537
		{ -0.0440040976f, 0.446780324f, -0.893560827f, 1669.87439f }, // source node 4230
		{ 1.22460635e-16f, 1.0f, -0.0f, 4848.0f }, // source node 4259
		{ -0.0f, -0.0f, -1.0f, -336.0f }, // source node 4266
		{ -1.22460635e-16f, -1.0f, -0.0f, -3511.99805f }, // source node 4286
		{ 0.258819371f, -0.965925753f, 0.0f, -3813.75024f }, // source node 4361
		{ -0.0f, -0.0f, -1.0f, -336.0f }, // source node 4387
	};
	struct HullRow { std::vector<int> entries; float bbox[6]; };

	// Each hull lists indices into Planes (with the flip bit where the stock data has it) and its bounding box.
	const HullRow Hulls[] = {
		{ { 0, 1 | Flip, 2, 3 | Flip, 4, 5 }, { 11.7097549f, 3522.71021f, 376.0f, 24.0008545f, 3544.0f, 377.0f } }, // 6 planes
		{ { 1 | Flip, 6, 2, 7 | Flip, 3, 8 }, { 24.0f, 3501.27637f, 376.0f, 32.0008545f, 3538.71826f, 377.0f } }, // 6 planes
		{ { 9 | Flip, 1 | Flip, 10, 6, 7, 11 }, { 21.2734375f, 3493.4707f, 376.0f, 32.0008545f, 3515.13672f, 377.0f } }, // 6 planes
		{ { 0, 12 | Flip, 13, 14 | Flip, 15, 16 }, { 11.7097425f, 3522.70996f, 383.0f, 24.0008545f, 3544.0f, 384.0f } }, // 6 planes
		{ { 12 | Flip, 17, 13, 18 | Flip, 14, 19 }, { 24.0f, 3501.27637f, 383.0f, 32.0008545f, 3538.71606f, 384.0f } }, // 6 planes
		{ { 9 | Flip, 12 | Flip, 20, 17, 18, 21 }, { 21.275156f, 3493.47119f, 383.0f, 32.0008545f, 3515.13672f, 384.0f } }, // 6 planes
		{ { 1, 12, 22 | Flip, 23, 24 | Flip, 25 }, { 16.3203125f, 3522.71313f, 377.0f, 24.0008545f, 3543.59497f, 383.0f } }, // 6 planes
		{ { 1, 12, 26, 23, 27 | Flip, 24 }, { 24.0008545f, 3501.27759f, 377.0f, 32.0008545f, 3538.71606f, 383.0f } }, // 6 planes
		{ { 9 | Flip, 1, 12, 28, 26, 27 }, { 21.2750626f, 3493.47095f, 377.0f, 32.0008545f, 3515.13672f, 383.0f } }, // 6 planes
		{ { 29, 30, 31 | Flip, 32, 33 }, { -32768.0f, 1868.0f, 336.0f, 876.689209f, 4848.0f, 2168.89868f } }, // 5 planes
		{ { 30, 31, 34, 35, 36 }, { -32768.0f, 3511.99634f, 336.0f, 19532.2383f, 32768.0f, 32768.0f } }, // 5 planes
	};

	constexpr size_t HullCount = std::size(Hulls);
	constexpr size_t PlaneCount = std::size(Planes);

	UModel* model = GC::Alloc<UModel>("ZonesPerimeterHulls", nullptr, ObjectFlags::NoFlags);

	// Nodes 0 to HullCount - 1 are the chain: far planes (x = 1000000), so the sweep sees
	// both ends on the same side and follows only Back. Each carries one hull.
	// The plane nodes follow, unlinked, and the hulls refer to them by index.
	for (size_t i = 0; i < HullCount; i++)
	{
		BspNode link = MakeNode(1.0f, 0.0f, 0.0f, 1000000.0f);
		if (i + 1 < HullCount)
			link.Back = (int)(i + 1);
		model->Nodes.push_back(link);
	}
	for (const PlaneRow& plane : Planes)
		model->Nodes.push_back(MakeNode(plane.x, plane.y, plane.z, plane.w));

	for (size_t i = 0; i < HullCount; i++)
	{
		model->Nodes[i].CollisionBound = (int)model->LeafHulls.size();

		std::vector<int> entries;
		for (int entry : Hulls[i].entries)
		{
			int planeIndex = entry & ~Flip;
			CHECK(planeIndex >= 0 && (size_t)planeIndex < PlaneCount);
			entries.push_back((planeIndex + (int)HullCount) | (entry & Flip));
		}
		AppendHull(model->LeafHulls, entries, Hulls[i].bbox);
	}

	// Three of the hulls carry the slab's top plane at z 384 and the box lands on it after
	// 21 units; the others hold lower planes, so the first hit is the slab's top.
	CollisionHitList hits = SweepDown(model, 28.0, 3516.0, 444.0);
	CHECK(!hits.empty());
	CHECK(std::abs(hits.front().Fraction - 21.0f) <= 0.3f);
	CHECK(hits.front().Normal.z > 0.9f);
}
