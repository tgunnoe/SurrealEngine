#include "Precomp.h"
#include "TestRunner.h"
#include "VM/Iterator.h"
#include "Packages/Core/UClass.h"
#include "Packages/Engine/Actors/UActor.h"
#include "Packages/Engine/Resources/Level/ULevel.h"
#include "PropertyOffsets.h"
#include "Utils/AlignedAlloc.h"
#include <string>
#include <vector>

// The engine global, declared here so the test can check it stays null without the engine header.
class Engine;
extern Engine* engine;

// These cover which levels an actor iterator walks and how the walk of a level list proceeds.
// A level is chosen from three handles (the calling actor's, the entry level's and the played
// level's) that the choice never dereferences, so those are opaque numbered keys. The walks run
// over levels and actors built by hand, with no package and no engine. How a script's calling
// actor reaches its level, and the iterators that look inside a level (radius, sight, collision,
// zone, traces), need a loaded package and are checked by running maps.

namespace
{
	constexpr uintptr_t LevelBase = 0x20000;

	// Handles only: the choice of levels must never dereference one, so these point at nothing.
	ULevel* Handle(int number)
	{
		return reinterpret_cast<ULevel*>(LevelBase + (uintptr_t)number * 0x100);
	}

	ULevel* const None = nullptr;
	ULevel* const E = Handle(1);
	ULevel* const P = Handle(2);
	ULevel* const X = Handle(3);

	using Levels = std::vector<ULevel*>;

	std::string Show(const Levels& levels)
	{
		std::string text = "{";
		for (ULevel* level : levels)
			text += (level == E ? " E" : level == P ? " P" : level == X ? " X" : " none");
		return text + " }";
	}

	struct World
	{
		UClass* ActorClass = GC::Alloc<UClass>("Actor", nullptr, ObjectFlags::NoFlags);
		UClass* PawnClass = GC::Alloc<UClass>("Pawn", nullptr, ObjectFlags::NoFlags);
		UClass* LightClass = GC::Alloc<UClass>("Light", nullptr, ObjectFlags::NoFlags);
		int Made = 0;

		World()
		{
			PawnClass->BaseStruct = ActorClass;
			LightClass->BaseStruct = ActorClass;
		}

		ULevel* NewLevel()
		{
			return GC::Alloc<ULevel>("TestLevel" + std::to_string(Made++), nullptr, ObjectFlags::NoFlags);
		}

		UActor* Add(ULevel* level, UClass* cls)
		{
			UActor* actor = GC::Alloc<UActor>("TestActor" + std::to_string(Made++), cls, ObjectFlags::NoFlags);
			level->Actors.push_back(actor);
			return actor;
		}
	};

	// Walks an iterator to its end, collecting what it yields. The bound comes from the fixture, so
	// an iterator that never ends fails the check on the count instead of running into the timeout.
	std::vector<UObject*> Drain(AllActorsIterator& iterator, UObject** slot, size_t bound)
	{
		std::vector<UObject*> yielded;
		while (yielded.size() <= bound && iterator.Next())
			yielded.push_back(*slot);
		return yielded;
	}
}

TEST(IteratorLevelsForAPlayedLevelCallerIsThePlayedLevel)
{
	CHECK(IteratorLevels(P, E, P) == Levels({ P }));
}

TEST(IteratorLevelsForAnEntryLevelCallerIsTheEntryLevel)
{
	Levels got = IteratorLevels(E, E, P);
	CHECK_MSG(got == Levels({ E }), "the entry level's own actors must walk the entry level, got " + Show(got));
}

TEST(IteratorLevelsWithoutAllLevelsIsTheCallersLevelAlone)
{
	for (ULevel* caller : { None, E, P, X })
		for (ULevel* entry : { None, E })
			for (ULevel* played : { None, P })
			{
				Levels got = IteratorLevels(caller, entry, played);
				Levels want = caller ? Levels({ caller }) : Levels();
				CHECK_MSG(got == want, "caller " + Show({ caller }) + " entry " + Show({ entry }) + " played " + Show({ played }) + " gave " + Show(got));
			}
}

TEST(IteratorLevelsWithNoEntryLevelIsTheCallersLevel)
{
	CHECK(IteratorLevels(P, None, P) == Levels({ P }));
}

TEST(IteratorLevelsForACallerInNoLevelIsNothingNotTheEnginesLevel)
{
	Levels got = IteratorLevels(None, E, P);
	CHECK_MSG(got.empty(), "an actor in no level must walk nothing, got " + Show(got));
}

TEST(AllActorsIteratorWalksOnlyTheLevelItIsGiven)
{
	World world;
	ULevel* a = world.NewLevel();
	ULevel* b = world.NewLevel();
	UActor* a0 = world.Add(a, world.ActorClass);
	UActor* a1 = world.Add(a, world.ActorClass);
	UActor* a2 = world.Add(a, world.ActorClass);
	UActor* b0 = world.Add(b, world.ActorClass);
	UActor* b1 = world.Add(b, world.ActorClass);

	UObject* slot = nullptr;
	AllActorsIterator overA({ a }, world.ActorClass, &slot, std::string());
	CHECK(Drain(overA, &slot, 8) == std::vector<UObject*>({ a0, a1, a2 }));
	CHECK(slot == nullptr);

	AllActorsIterator overB({ b }, world.ActorClass, &slot, std::string());
	CHECK(Drain(overB, &slot, 8) == std::vector<UObject*>({ b0, b1 }));
	CHECK(slot == nullptr);
}

TEST(AllActorsIteratorWalksTheLevelsInTheOrderGiven)
{
	World world;
	ULevel* a = world.NewLevel();
	ULevel* b = world.NewLevel();
	UActor* a0 = world.Add(a, world.ActorClass);
	UActor* a1 = world.Add(a, world.ActorClass);
	UActor* a2 = world.Add(a, world.ActorClass);
	UActor* b0 = world.Add(b, world.ActorClass);
	UActor* b1 = world.Add(b, world.ActorClass);

	UObject* slot = nullptr;
	AllActorsIterator ab({ a, b }, world.ActorClass, &slot, std::string());
	CHECK(Drain(ab, &slot, 8) == std::vector<UObject*>({ a0, a1, a2, b0, b1 }));

	AllActorsIterator ba({ b, a }, world.ActorClass, &slot, std::string());
	CHECK(Drain(ba, &slot, 8) == std::vector<UObject*>({ b0, b1, a0, a1, a2 }));
}

TEST(AllActorsIteratorKeepsOnlyActorsOfTheClassAsked)
{
	World world;
	ULevel* level = world.NewLevel();
	UActor* plain = world.Add(level, world.ActorClass);
	level->Actors.push_back(nullptr);
	UActor* pawn = world.Add(level, world.PawnClass);
	UActor* light = world.Add(level, world.LightClass);

	UObject* slot = nullptr;
	AllActorsIterator pawns({ level }, world.PawnClass, &slot, std::string());
	CHECK(Drain(pawns, &slot, 8) == std::vector<UObject*>({ pawn }));

	AllActorsIterator actors({ level }, world.ActorClass, &slot, std::string());
	CHECK(Drain(actors, &slot, 8) == std::vector<UObject*>({ plain, pawn, light }));
}

TEST(AllActorsIteratorSeesAnActorAddedWhileItWalks)
{
	World world;
	ULevel* level = world.NewLevel();
	UActor* c0 = world.Add(level, world.ActorClass);

	UObject* slot = nullptr;
	AllActorsIterator iterator({ level }, world.ActorClass, &slot, std::string());
	CHECK(iterator.Next());
	CHECK(slot == c0);

	UActor* c1 = world.Add(level, world.ActorClass);
	CHECK(iterator.Next());
	CHECK(slot == c1);
	CHECK(!iterator.Next());
	CHECK(slot == nullptr);
}

TEST(AllActorsIteratorOverNoLevelEndsAtOnce)
{
	World world;
	UActor* stray = world.Add(world.NewLevel(), world.ActorClass);

	UObject* slot = stray;
	AllActorsIterator iterator(Levels(), world.ActorClass, &slot, std::string());
	CHECK(!iterator.Next());
	CHECK(slot == nullptr);
}

TEST(CycleActorsIteratorNumbersTheMatchesAcrossTheLevelsGiven)
{
	World world;
	ULevel* a = world.NewLevel();
	ULevel* b = world.NewLevel();
	UActor* a0 = world.Add(a, world.ActorClass);
	UActor* a1 = world.Add(a, world.ActorClass);
	UActor* a2 = world.Add(a, world.ActorClass);
	UActor* b0 = world.Add(b, world.ActorClass);
	UActor* b1 = world.Add(b, world.ActorClass);

	auto run = [&](Levels levels, std::vector<UObject*>& yielded, std::vector<int>& indexes)
	{
		UObject* slot = nullptr;
		int index = -1;
		CycleActorsIterator iterator(levels, world.ActorClass, &slot, &index);
		while (yielded.size() <= 8 && iterator.Next())
		{
			yielded.push_back(slot);
			indexes.push_back(index);
		}
	};

	std::vector<UObject*> yielded;
	std::vector<int> indexes;
	run({ a, b }, yielded, indexes);
	CHECK(yielded == std::vector<UObject*>({ a0, a1, a2, b0, b1 }));
	CHECK(indexes == std::vector<int>({ 0, 1, 2, 3, 4 }));

	yielded.clear();
	indexes.clear();
	run({ b }, yielded, indexes);
	CHECK(yielded == std::vector<UObject*>({ b0, b1 }));
	CHECK(indexes == std::vector<int>({ 0, 1 }));
}

// Falling back to the played level for an actor in no level is the rule this branch removed.
TEST(ActorLevelsForAnActorInNoLevelIsNothingAndNeedsNoEngine)
{
	// The runner runs one test per process, so setting this global is safe.
	PropOffsets_Actor.XLevel.DataOffset = 0;
	UClass* cls = GC::Alloc<UClass>("Actor", nullptr, ObjectFlags::NoFlags);
	UActor* actor = GC::Alloc<UActor>("NoLevelActor", cls, ObjectFlags::NoFlags);
	actor->PropertyData.Data = AlignedAlloc(alignof(ULevel*), sizeof(ULevel*));
	actor->PropertyData.Size = sizeof(ULevel*);
	actor->XLevel() = nullptr;

	CHECK(engine == nullptr);
	std::vector<ULevel*> levels;
	try { levels = ActorLevels(actor, "AllActors"); }
	catch (...) { CHECK_MSG(false, "ActorLevels threw for an actor in no level"); }
	CHECK(levels.empty());
	CHECK(engine == nullptr);
}
