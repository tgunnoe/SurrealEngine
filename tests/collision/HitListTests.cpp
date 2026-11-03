#include "Precomp.h"
#include "TestRunner.h"
#include "Collision/TopLevel/CollisionHit.h"
#include <vector>

// A trace or an overlap test reports one hit per actor, but a world hit carries the level's
// LevelInfo as its actor and every world hit is a different surface, which the texture trace and
// the ladder test read. The function only compares actors, so actors here are opaque pointers.

namespace
{
	constexpr uintptr_t ActorBase = 0x10000;
	constexpr uintptr_t ActorStride = 0x100;

	// Keys only: these point at nothing.
	UActor* Actor(int number)
	{
		return reinterpret_cast<UActor*>(ActorBase + (uintptr_t)number * ActorStride);
	}

	CollisionHit Hit(float fraction, UActor* actor)
	{
		return CollisionHit(fraction, vec3(0.0f), actor, nullptr, nullptr);
	}

	struct Kept
	{
		float Fraction;
		UActor* Actor;
	};

	std::vector<Kept> KeptHits(const CollisionHitList& hits)
	{
		std::vector<Kept> kept;
		for (const CollisionHit& hit : hits)
			kept.push_back({ hit.Fraction, hit.Actor });
		return kept;
	}
}

TEST(UniqueActorHitsKeepEveryWorldHit)
{
	UActor* levelInfo = Actor(1);
	UActor* pawn = Actor(2);

	// In fraction order, as a trace sorts them. A list holds a hit with no actor only at fraction 1.
	CollisionHitList hits;
	hits.push_back(Hit(0.2f, levelInfo));
	hits.push_back(Hit(0.3f, pawn));
	hits.push_back(Hit(0.6f, levelInfo));
	hits.push_back(Hit(0.7f, pawn));
	hits.push_back(Hit(1.0f, nullptr));
	hits.push_back(Hit(1.0f, nullptr));

	std::vector<Kept> kept = KeptHits(UniqueActorHits(hits, levelInfo));
	CHECK_EQ(kept.size(), (size_t)5);

	// Both world hits stay, in order
	CHECK(kept[0].Actor == levelInfo && kept[0].Fraction == 0.2f);
	CHECK(kept[2].Actor == levelInfo && kept[2].Fraction == 0.6f);

	// The pawn keeps only its nearer hit
	CHECK(kept[1].Actor == pawn && kept[1].Fraction == 0.3f);

	// Every hit with no actor stays
	CHECK(kept[3].Actor == nullptr && kept[4].Actor == nullptr);
}
