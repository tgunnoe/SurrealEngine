#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/DeusEx/UScriptedPawn.h"

// These cover the rule behind ScriptedPawn.IsValidEnemy. The native itself needs a loaded level and is
// checked by running maps; here the engine-side inputs (the candidate's health, its bDetectable flag,
// the caller's alliance toward it, and whether the script passed the alliance flag) are plain booleans.

namespace
{
	// A living, detectable pawn other than the caller, hostile to it, with the alliance flag omitted
	// (which the script engine reads as true).
	bool PlayerWithFlagOmitted()
	{
		return EnemyValidity(true, false, true, true, true, true);
	}
}

TEST(EnemyValidityHostilePlayerIsValidWithFlagOmitted)
{
	CHECK(PlayerWithFlagOmitted());
}

TEST(EnemyValidityFriendIsInvalidWhenAllianceIsChecked)
{
	CHECK(!EnemyValidity(true, false, true, true, true, false));
}

TEST(EnemyValidityFriendIsValidWhenAllianceCheckIsOff)
{
	CHECK(EnemyValidity(true, false, true, true, false, false));
}

TEST(EnemyValidityHostilePawnStaysValidWhenAllianceCheckIsOff)
{
	CHECK(EnemyValidity(true, false, true, true, false, true));
}

TEST(EnemyValidityMissingCandidateIsInvalid)
{
	for (bool checkAlliance : { false, true })
		for (bool hostile : { false, true })
			CHECK(!EnemyValidity(false, false, true, true, checkAlliance, hostile));
}

TEST(EnemyValiditySelfIsInvalid)
{
	for (bool checkAlliance : { false, true })
		for (bool hostile : { false, true })
			CHECK(!EnemyValidity(true, true, true, true, checkAlliance, hostile));
}

TEST(EnemyValidityDeadPawnIsInvalid)
{
	for (bool checkAlliance : { false, true })
		for (bool hostile : { false, true })
			CHECK(!EnemyValidity(true, false, false, true, checkAlliance, hostile));
}

TEST(EnemyValidityUndetectablePawnIsInvalid)
{
	for (bool checkAlliance : { false, true })
		for (bool hostile : { false, true })
			CHECK(!EnemyValidity(true, false, true, false, checkAlliance, hostile));
}

// Every combination of the six inputs, checked against the rule's three implications: a valid enemy
// passes the four gates, a valid enemy is hostile whenever the alliance is checked, and nothing else
// stands in the way of a pawn that passes all of those.
TEST(EnemyValidityAllInputCombinations)
{
	for (int bits = 0; bits < 64; bits++)
	{
		bool candidateExists = (bits & 1) != 0;
		bool isSelf = (bits & 2) != 0;
		bool alive = (bits & 4) != 0;
		bool detectable = (bits & 8) != 0;
		bool checkAlliance = (bits & 16) != 0;
		bool hostile = (bits & 32) != 0;

		bool valid = EnemyValidity(candidateExists, isSelf, alive, detectable, checkAlliance, hostile);
		bool passesGates = candidateExists && !isSelf && alive && detectable;
		bool passesAlliance = !checkAlliance || hostile;

		CHECK_MSG(!valid || passesGates, "inputs " + std::to_string(bits) + ": valid but failed a gate");
		CHECK_MSG(!valid || passesAlliance, "inputs " + std::to_string(bits) + ": valid but not hostile under an alliance check");
		CHECK_MSG(valid || !(passesGates && passesAlliance), "inputs " + std::to_string(bits) + ": passes every condition but was refused");
	}
}
