#include "Precomp.h"
#include "TestRunner.h"
#include "Collision/TopLevel/Encroachment.h"

// A moving brush meets every actor that overlaps it. The broad phase looks at the grid cells that
// the brush's world box covers, and the gather keeps each non-brush candidate that the caller's
// overlap predicate accepts. The world below is the test's own: actors are cylinders registered
// in every 256-unit cell their box covers, and the brush's world box is worked out here with
// plain trigonometry rather than the engine's matrices.

namespace
{
	// A mover's brush and where it stands: object space to world space is a translation by the
	// location after a rotation about z after a scale, all about the pre-pivot.
	struct Pose
	{
		BBox box;
		vec3 location;
		double yaw = 0.0;
		vec3 scale = vec3(1.0f);
		vec3 prePivot = vec3(0.0f);
	};

	struct WorldBox
	{
		double lo[3];
		double hi[3];
	};

	WorldBox WorldBoxOf(const Pose& pose)
	{
		double c = std::cos(pose.yaw);
		double s = std::sin(pose.yaw);
		WorldBox result = { { 1e30, 1e30, 1e30 }, { -1e30, -1e30, -1e30 } };
		for (int i = 0; i < 8; i++)
		{
			double x = ((i & 1) ? pose.box.max.x : pose.box.min.x) - pose.prePivot.x;
			double y = ((i & 2) ? pose.box.max.y : pose.box.min.y) - pose.prePivot.y;
			double z = ((i & 4) ? pose.box.max.z : pose.box.min.z) - pose.prePivot.z;
			x *= pose.scale.x;
			y *= pose.scale.y;
			z *= pose.scale.z;
			double w[3] = { c * x - s * y + pose.location.x, s * x + c * y + pose.location.y, z + pose.location.z };
			for (int axis = 0; axis < 3; axis++)
			{
				result.lo[axis] = std::min(result.lo[axis], w[axis]);
				result.hi[axis] = std::max(result.hi[axis], w[axis]);
			}
		}
		return result;
	}

	// The mover as the window function is given it: the same pose in the engine's terms, and the
	// collision cylinder the mover carries beside its brush.
	EncroachWindowSource SourceOf(const Pose& pose, float radius, float height)
	{
		float c = (float)std::cos(pose.yaw);
		float s = (float)std::sin(pose.yaw);
		mat4 rotation = mat4::identity();
		rotation[0 + 0 * 4] = c;
		rotation[1 + 0 * 4] = s;
		rotation[0 + 1 * 4] = -s;
		rotation[1 + 1 * 4] = c;

		EncroachWindowSource source;
		source.brushBox = pose.box;
		source.location = pose.location;
		source.rotation = rotation;
		source.mainScale = pose.scale;
		source.prePivot = pose.prePivot;
		source.radius = radius;
		source.height = height;
		return source;
	}

	// An actor as the grid sees it
	struct Placed
	{
		int id;
		bool isMover;
		bool isBrush;
		double location[3];
		double radius;
		double height;
	};

	double Extent(const Placed& p, int axis)
	{
		return axis == 2 ? p.height : p.radius;
	}

	int CellOf(double coordinate)
	{
		return (int)std::floor(coordinate / 256.0);
	}

	bool Registered(const Placed& p, int x, int y, int z)
	{
		int cell[3] = { x, y, z };
		for (int axis = 0; axis < 3; axis++)
		{
			if (cell[axis] < CellOf(p.location[axis] - Extent(p, axis)) || cell[axis] > CellOf(p.location[axis] + Extent(p, axis)))
				return false;
		}
		return true;
	}

	bool WindowCoversCellOf(const EncroachCellRange& window, const Placed& p)
	{
		int cell[3] = { CellOf(p.location[0]), CellOf(p.location[1]), CellOf(p.location[2]) };
		int start[3] = { window.start.x, window.start.y, window.start.z };
		int end[3] = { window.end.x, window.end.y, window.end.z };
		for (int axis = 0; axis < 3; axis++)
		{
			if (cell[axis] < start[axis] || cell[axis] >= end[axis])
				return false;
		}
		return true;
	}

	// The records the caller would build: the actors of each window cell in cell order (z, then y,
	// then x), an actor that spans several cells once for each.
	std::vector<EncroachCandidate<int>> CandidatesOf(const EncroachWindowSource& source, const std::vector<Placed>& world)
	{
		EncroachCellRange window = EncroachWindow(source);
		std::vector<EncroachCandidate<int>> candidates;
		for (int z = window.start.z; z < window.end.z; z++)
		{
			for (int y = window.start.y; y < window.end.y; y++)
			{
				for (int x = window.start.x; x < window.end.x; x++)
				{
					for (const Placed& p : world)
					{
						if (Registered(p, x, y, z))
							candidates.push_back({ p.id, p.isMover, p.isBrush });
					}
				}
			}
		}
		return candidates;
	}

	bool InsideWorldBox(const WorldBox& box, const Placed& p)
	{
		for (int axis = 0; axis < 3; axis++)
		{
			if (p.location[axis] + Extent(p, axis) < box.lo[axis] || p.location[axis] - Extent(p, axis) > box.hi[axis])
				return false;
		}
		return true;
	}

	bool Contains(const std::vector<int>& ids, int id)
	{
		return std::find(ids.begin(), ids.end(), id) != ids.end();
	}

	Placed Actor(int id, double x, double y, double z, double radius, double height)
	{
		return { id, false, false, { x, y, z }, radius, height };
	}

	// A door-sized brush: 200 units each way, centred on the mover, with a collision cylinder that
	// is smaller than it.
	Pose BlockPose()
	{
		return { BBox(vec3(-100.0f), vec3(100.0f)), vec3(0.0f) };
	}

	// A wall-like slab, 1400 units long, with a collision cylinder that covers a small part of it.
	Pose SlabPose()
	{
		return { BBox(vec3(-700.0f, -20.0f, -100.0f), vec3(700.0f, 20.0f, 100.0f)), vec3(0.0f) };
	}
}

TEST(EncroachGathersEveryOverlappingCandidate)
{
	Pose pose = BlockPose();
	EncroachWindowSource source = SourceOf(pose, 64.0f, 64.0f);
	WorldBox box = WorldBoxOf(pose);

	std::vector<Placed> world = {
		Actor(1, 10.0, 10.0, 0.0, 17.0, 40.0),
		Actor(2, -20.0, 30.0, 10.0, 17.0, 40.0),
		Actor(3, 200.0, 0.0, 0.0, 17.0, 40.0) }; // in the window, clear of the brush
	CHECK(InsideWorldBox(box, world[0]) && InsideWorldBox(box, world[1]) && !InsideWorldBox(box, world[2]));

	std::vector<int> hits = GatherEncroaching(CandidatesOf(source, world), [&](int id) { return InsideWorldBox(box, world[id - 1]); });

	CHECK_EQ(hits.size(), (size_t)2);
	CHECK(Contains(hits, 1) && Contains(hits, 2));
}

TEST(EncroachNeverReturnsTheMoverOrABrush)
{
	Pose pose = BlockPose();
	EncroachWindowSource source = SourceOf(pose, 64.0f, 64.0f);

	// Both overlap, and the predicate accepts everything, so only their flags can keep them out.
	std::vector<Placed> world = {
		{ 1, true, false, { 0.0, 0.0, 0.0 }, 64.0, 64.0 },
		{ 2, false, true, { 20.0, 20.0, 0.0 }, 30.0, 30.0 } };
	std::vector<EncroachCandidate<int>> candidates = CandidatesOf(source, world);
	CHECK(!candidates.empty());

	std::vector<int> hits = GatherEncroaching(candidates, [](int) { return true; });

	CHECK(hits.empty());
}

TEST(EncroachReturnsACandidateInTwoCellsOnce)
{
	Pose pose = BlockPose();
	EncroachWindowSource source = SourceOf(pose, 64.0f, 64.0f);
	WorldBox box = WorldBoxOf(pose);

	// Straddles the plane x = 0, so the grid lists it in the cell on each side.
	std::vector<Placed> world = { Actor(1, 0.0, 50.0, 50.0, 20.0, 20.0) };
	std::vector<EncroachCandidate<int>> candidates = CandidatesOf(source, world);
	CHECK_EQ(candidates.size(), (size_t)2);

	std::vector<int> hits = GatherEncroaching(candidates, [&](int id) { return InsideWorldBox(box, world[id - 1]); });

	CHECK_EQ(hits.size(), (size_t)1);
	CHECK_EQ(hits[0], 1);
}

TEST(EncroachWindowReachesBrushBeyondTheCylinder)
{
	Pose pose = SlabPose();
	EncroachWindowSource source = SourceOf(pose, 40.0f, 40.0f);
	WorldBox box = WorldBoxOf(pose);

	// Well inside the slab, 500 units along it, where the cylinder of radius 40 does not reach.
	std::vector<Placed> world = { Actor(1, 500.0, 0.0, 0.0, 17.0, 40.0) };
	CHECK(InsideWorldBox(box, world[0]));
	CHECK(world[0].location[0] - world[0].radius > source.radius);

	std::vector<int> hits = GatherEncroaching(CandidatesOf(source, world), [&](int id) { return InsideWorldBox(box, world[id - 1]); });

	CHECK_EQ(hits.size(), (size_t)1);
	CHECK_EQ(hits[0], 1);
}

TEST(EncroachWindowExcludesCandidatesBeyondTheBrushBox)
{
	Pose pose = SlabPose();
	EncroachWindowSource source = SourceOf(pose, 40.0f, 40.0f);
	WorldBox box = WorldBoxOf(pose);

	// Each lies in a cell past one face of the world box (x past the long end, x past the near
	// end, z above the top). The predicate accepts everything, so only the window can exclude them.
	std::vector<Placed> world = {
		Actor(1, 1000.0, 0.0, 0.0, 17.0, 40.0),
		Actor(2, -900.0, 0.0, 0.0, 17.0, 40.0),
		Actor(3, 0.0, 0.0, 300.0, 17.0, 40.0) };
	EncroachCellRange window = EncroachWindow(source);
	for (const Placed& p : world)
	{
		CHECK(!InsideWorldBox(box, p));
		CHECK(!WindowCoversCellOf(window, p));
	}

	std::vector<int> hits = GatherEncroaching(CandidatesOf(source, world), [](int) { return true; });

	CHECK(hits.empty());
}

TEST(EncroachWindowFollowsRotationAndScale)
{
	Pose upright = BlockPose();
	Pose turned = BlockPose();
	turned.yaw = std::acos(-1.0) / 4.0;
	turned.scale = vec3(2.0f, 2.0f, 1.0f);

	// Turned a quarter of a right angle and doubled across, the brush's corner reaches about
	// 283 units along x, into the next cell, where the upright brush ends at 100.
	std::vector<Placed> world = { Actor(1, 270.0, 0.0, 0.0, 10.0, 40.0) };
	CHECK(InsideWorldBox(WorldBoxOf(turned), world[0]) && !InsideWorldBox(WorldBoxOf(upright), world[0]));
	CHECK(!WindowCoversCellOf(EncroachWindow(SourceOf(upright, 40.0f, 40.0f)), world[0]));

	EncroachWindowSource source = SourceOf(turned, 40.0f, 40.0f);
	WorldBox box = WorldBoxOf(turned);
	std::vector<int> hits = GatherEncroaching(CandidatesOf(source, world), [&](int id) { return InsideWorldBox(box, world[id - 1]); });

	CHECK_EQ(hits.size(), (size_t)1);
	CHECK_EQ(hits[0], 1);

	EncroachCellRange window = EncroachWindow(source);
	CHECK(window.start == ivec3(-2, -2, -1) && window.end == ivec3(2, 2, 1));
}
