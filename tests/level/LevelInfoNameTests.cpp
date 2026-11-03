#include "Precomp.h"
#include "TestRunner.h"
#include "Engine.h"
#include <string>
#include <vector>

// These cover which of a map's level info objects the game binds to. That the pick reaches the actors
// of a loaded map, which hold the level info the map saved, needs a running engine and is checked by
// the smoke runs of Tools/smoke.

TEST(LevelInfoNameIsTheFirstActorWhenItIsALevelInfo)
{
	// ExtremeDGen: the first actor is LevelInfo5 and the name search would find LevelInfo1,
	// an object the map saved as deleted
	CHECK_EQ(LevelInfoName(true, "LevelInfo5", { "LevelInfo1", "LevelInfo3", "LevelInfo5", "LevelInfo6" }, 226), "LevelInfo5");
	CHECK_EQ(LevelInfoName(true, "LevelInfo0", { "LevelInfo0", "LevelInfo1", "LevelInfo2" }, 226), "LevelInfo0");
	CHECK_EQ(LevelInfoName(true, "LevelInfo0", { "LevelInfo0" }, 436), "LevelInfo0");
	// Even a LevelInfo0 that the map holds does not outrank the first actor
	CHECK_EQ(LevelInfoName(true, "LevelInfo2", { "LevelInfo0", "LevelInfo2" }, 226), "LevelInfo2");
}

TEST(LevelInfoNameIgnoresTheFirstActorNameWhenItIsNotALevelInfo)
{
	// The name only counts when the caller found the first actor to be a level info
	CHECK_EQ(LevelInfoName(false, "Brush0", { "LevelInfo0", "LevelInfo3" }, 226), "LevelInfo0");
}

TEST(LevelInfoNameFallsBackToLevelInfo0First)
{
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo3", "LevelInfo0", "LevelInfo1" }, 226), "LevelInfo0");
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo0", "LevelInfo1" }, 436), "LevelInfo0");
}

TEST(LevelInfoNameFallsBackToTheLowestNumberedBelowVersion300)
{
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo1", "LevelInfo3", "LevelInfo5", "LevelInfo6" }, 226), "LevelInfo1");
	// Numeric order, whatever order the names come in
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo12", "LevelInfo2" }, 226), "LevelInfo2");
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo2", "LevelInfo12" }, 226), "LevelInfo2");
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo19" }, 226), "LevelInfo19");
}

TEST(LevelInfoNameIsEmptyWhenNothingMatches)
{
	// From version 300 only LevelInfo0 is searched for
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo1", "LevelInfo3", "LevelInfo5", "LevelInfo6" }, 436), "");
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo1" }, 300), "");
	CHECK_EQ(LevelInfoName(false, "", {}, 226), "");
	// The search stops at LevelInfo19, and a name that is not a level info's is not one
	CHECK_EQ(LevelInfoName(false, "", { "LevelInfo20" }, 226), "");
	CHECK_EQ(LevelInfoName(false, "", { "Brush0" }, 226), "");
}
