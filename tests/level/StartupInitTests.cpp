#include "Precomp.h"
#include "TestRunner.h"
#include "StartupInit.h"

// These cover which actors the begin-play loops of a map load leave alone. That the loops call the
// decision for every loop but the Deus Ex one, and what the skip does to a loaded map, needs a running
// engine and is checked by the load runs of Tools/smoke/scenarios/beginplay.

namespace
{
	const size_t LoadCount = 100;

	// Counts the reads of the flag, which an absent property must never see
	struct CountingFlag
	{
		bool Set = false;
		int Calls = 0;
		bool operator()() { Calls++; return Set; }
	};

	PropertyDataOffset PresentOffset()
	{
		PropertyDataOffset offset;
		offset.DataOffset = 12;
		return offset;
	}
}

TEST(LoadBeginPlayVisitsAPlacedActorWhoseFlagIsSet)
{
	CountingFlag flag;
	flag.Set = true;
	CHECK(!SkipBeginPlayAtLoad(0, LoadCount, PresentOffset(), flag));
	CHECK(!SkipBeginPlayAtLoad(LoadCount / 2, LoadCount, PresentOffset(), flag));
	CHECK(!SkipBeginPlayAtLoad(LoadCount - 1, LoadCount, PresentOffset(), flag));
	flag.Set = false;
	CHECK(!SkipBeginPlayAtLoad(LoadCount - 1, LoadCount, PresentOffset(), flag));
}

TEST(LoadBeginPlaySkipsASpawnedActorWhoseFlagIsSet)
{
	CountingFlag flag;
	flag.Set = true;
	CHECK(SkipBeginPlayAtLoad(LoadCount + 50, LoadCount, PresentOffset(), flag));
	CHECK(SkipBeginPlayAtLoad(LoadCount + 5000, LoadCount, PresentOffset(), flag));
}

TEST(LoadBeginPlaySkipsTheFirstSpawnedIndexAndVisitsTheLastPlacedOne)
{
	CountingFlag flag;
	flag.Set = true;
	CHECK(SkipBeginPlayAtLoad(LoadCount, LoadCount, PresentOffset(), flag));
	CHECK(!SkipBeginPlayAtLoad(LoadCount - 1, LoadCount, PresentOffset(), flag));
}

TEST(LoadBeginPlayVisitsASpawnedActorWhoseFlagIsClear)
{
	CountingFlag flag;
	flag.Set = false;
	CHECK(!SkipBeginPlayAtLoad(LoadCount, LoadCount, PresentOffset(), flag));
	CHECK(!SkipBeginPlayAtLoad(LoadCount + 50, LoadCount, PresentOffset(), flag));
}

TEST(LoadBeginPlayNeverReadsTheFlagWhenTheOffsetIsAbsent)
{
	// The default offset is the one an actor class without the property resolves to
	PropertyDataOffset absent;
	CHECK_EQ(absent.DataOffset, ~(size_t)0);
	CountingFlag flag;
	flag.Set = true;
	CHECK(!SkipBeginPlayAtLoad(0, LoadCount, absent, flag));
	CHECK(!SkipBeginPlayAtLoad(LoadCount - 1, LoadCount, absent, flag));
	CHECK(!SkipBeginPlayAtLoad(LoadCount, LoadCount, absent, flag));
	CHECK(!SkipBeginPlayAtLoad(LoadCount + 50, LoadCount, absent, flag));
	CHECK_EQ(flag.Calls, 0);
}
