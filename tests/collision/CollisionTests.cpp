#include "Precomp.h"
#include "TestRunner.h"
#include "Collision/BottomLevel/TraceAABBModel.h"
#include "Collision/BottomLevel/TraceRayModel.h"
#include "Collision/BottomLevel/OverlapAABBModel.h"
#include "Packages/Engine/Resources/Level/UModel.h"

// A mover whose brush is missing or has no BSP nodes reaches the bottom level as a null
// model or a node-less one; both must read as "nothing to hit" rather than dereference
// the first node.

namespace
{
	const dvec3 Origin(0.0, 0.0, 0.0);
	const dvec3 Direction(0.0, 0.0, -1.0);
	const dvec3 Extents(16.0, 16.0, 16.0);
}

TEST(CollisionTraceAABBNullModel)
{
	TraceAABBModel tracer;
	CHECK(tracer.Trace(nullptr, Origin, 0.0, Direction, 100.0, Extents, false).empty());
}

TEST(CollisionTraceRayNullModel)
{
	TraceRayModel tracer;
	CHECK(tracer.Trace(nullptr, Origin, 0.0, Direction, 100.0, false).empty());
}

TEST(CollisionTraceRayAnyHitNullModel)
{
	TraceRayModel tracer;
	CHECK(!tracer.TraceAnyHit(nullptr, Origin, 0.0, Direction, 100.0, false));
}

TEST(CollisionOverlapAABBNullModel)
{
	OverlapAABBModel tester;
	CHECK(tester.TestOverlap(nullptr, vec3(0.0f), vec3(16.0f), false).empty());
}

TEST(CollisionTraceAABBEmptyModel)
{
	UModel* model = GC::Alloc<UModel>("EmptyModel", nullptr, ObjectFlags::NoFlags);
	TraceAABBModel tracer;
	CHECK(tracer.Trace(model, Origin, 0.0, Direction, 100.0, Extents, false).empty());
}

TEST(CollisionTraceRayEmptyModel)
{
	UModel* model = GC::Alloc<UModel>("EmptyModel", nullptr, ObjectFlags::NoFlags);
	TraceRayModel tracer;
	CHECK(tracer.Trace(model, Origin, 0.0, Direction, 100.0, false).empty());
}

TEST(CollisionTraceRayAnyHitEmptyModel)
{
	UModel* model = GC::Alloc<UModel>("EmptyModel", nullptr, ObjectFlags::NoFlags);
	TraceRayModel tracer;
	CHECK(!tracer.TraceAnyHit(model, Origin, 0.0, Direction, 100.0, false));
}

TEST(CollisionOverlapAABBEmptyModel)
{
	UModel* model = GC::Alloc<UModel>("EmptyModel", nullptr, ObjectFlags::NoFlags);
	OverlapAABBModel tester;
	CHECK(tester.TestOverlap(model, vec3(0.0f), vec3(16.0f), false).empty());
}
