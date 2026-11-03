#include "Precomp.h"
#include "TestRunner.h"
#include "Engine.h"
#include "Packages/Core/UClass.h"
#include <string>

// These cover how the call command finds the actor behind a "name:", "class:" or "bind:" argument,
// over a list of actor records. Reading the records off a running level is checked by running maps.

namespace
{
	struct Actors
	{
		UClass* Class = GC::Alloc<UClass>("TestClass", nullptr, ObjectFlags::NoFlags);
		Array<CallActorRecord> Records;

		UObject* Add(const std::string& name, const std::string& className = "Pawn", const std::string& bindName = "", bool deleted = false)
		{
			CallActorRecord record;
			record.object = GC::Alloc<UObject>(name, Class, ObjectFlags::NoFlags);
			record.name = name;
			record.className = className;
			record.bindName = bindName;
			record.location = vec3(1.5f, -2.0f, 300.0f);
			record.deleted = deleted;
			Records.push_back(record);
			return record.object;
		}

		UObject* AddDeleted(const std::string& name, const std::string& className = "Pawn", const std::string& bindName = "")
		{
			return Add(name, className, bindName, true);
		}
	};

	struct Answer
	{
		UObject* Object = nullptr;
		std::string Error;
		std::string Resolution;
	};

	Answer Resolve(const char* kind, const char* value, const Actors& actors, bool isDeusEx = false)
	{
		Answer answer;
		answer.Object = ResolveCallObject(kind, value, actors.Records, isDeusEx, answer.Error, answer.Resolution);
		return answer;
	}

	// Actors named Prefix00, Prefix01, ... all of one class
	void AddNumbered(Actors& actors, int count)
	{
		for (int i = 0; i < count; i++)
			actors.Add(std::string("Actor") + (i < 10 ? "0" : "") + std::to_string(i));
	}
}

TEST(HarnessResolverFindsAnActorByNameClassAndBindName)
{
	Actors actors;
	actors.Add("Alpha", "Pawn");
	UObject* paul = actors.Add("Beta", "ScriptedPawn", "PaulDenton");

	Answer byName = Resolve("name", "Beta", actors, true);
	CHECK(byName.Object == paul);
	CHECK_MSG(byName.Error.empty(), byName.Error);

	Answer byClass = Resolve("class", "ScriptedPawn", actors, true);
	CHECK(byClass.Object == paul);

	Answer byBind = Resolve("bind", "PaulDenton", actors, true);
	CHECK(byBind.Object == paul);
	CHECK_MSG(byBind.Error.empty(), byBind.Error);
}

TEST(HarnessResolverWritesTheResolutionLineWithTheTypedValueAndTheLocation)
{
	Actors actors;
	actors.Add("Beta", "ScriptedPawn", "PaulDenton");

	Answer answer = Resolve("bind", "paulDENTON", actors, true);
	CHECK(answer.Object != nullptr);
	CHECK_EQ(answer.Resolution, std::string("call: bind:paulDENTON resolved to Beta at (1.5, -2.0, 300.0)"));
}

TEST(HarnessResolverMatchesWithoutRegardToCase)
{
	Actors actors;
	UObject* paul = actors.Add("Alpha", "ScriptedPawn", "PaulDenton");

	CHECK(Resolve("name", "aLPHA", actors).Object == paul);
	CHECK(Resolve("class", "SCRIPTEDPAWN", actors).Object == paul);
	CHECK(Resolve("bind", "pauldenton", actors, true).Object == paul);
}

TEST(HarnessResolverTakesTheFirstActorThatMatches)
{
	Actors actors;
	UObject* first = actors.Add("Alpha", "Pawn");
	actors.Add("Beta", "Pawn");

	CHECK(Resolve("class", "Pawn", actors).Object == first);
}

TEST(HarnessResolverSkipsAnActorBeingDeletedInFavourOfALaterMatch)
{
	Actors actors;
	actors.AddDeleted("Alpha", "Pawn", "PaulDenton");
	UObject* live = actors.Add("Alpha", "Pawn", "PaulDenton");

	Answer byName = Resolve("name", "Alpha", actors, true);
	CHECK(byName.Object == live);
	CHECK(Resolve("class", "Pawn", actors, true).Object == live);
	CHECK(Resolve("bind", "PaulDenton", actors, true).Object == live);
}

TEST(HarnessResolverRefusesAMatchThatIsBeingDeletedAndDoesNotOfferIt)
{
	Actors actors;
	actors.AddDeleted("Ghost");
	actors.Add("Alpha");

	Answer answer = Resolve("name", "Ghost", actors);
	CHECK(answer.Object == nullptr);
	CHECK(answer.Resolution.empty());
	CHECK_EQ(answer.Error, std::string("no actor in the level has the actor name 'Ghost'; the level offers: Alpha"));
}

TEST(HarnessResolverNeedsADeusExGameForBindOnly)
{
	Actors actors;
	UObject* paul = actors.Add("Alpha", "ScriptedPawn", "PaulDenton");

	Answer refused = Resolve("bind", "PaulDenton", actors, false);
	CHECK(refused.Object == nullptr);
	CHECK_EQ(refused.Error, std::string("bind: needs a Deus Ex game"));
	CHECK(refused.Resolution.empty());

	// The other two forms need no particular game
	CHECK(Resolve("name", "Alpha", actors, false).Object == paul);
	CHECK(Resolve("class", "ScriptedPawn", actors, false).Object == paul);
	CHECK(Resolve("bind", "PaulDenton", actors, true).Object == paul);
}

TEST(HarnessResolverNamesTheFormItLookedFor)
{
	Actors actors;
	actors.Add("Alpha", "Pawn", "PaulDenton");

	CHECK_EQ(Resolve("name", "Zed", actors).Error, std::string("no actor in the level has the actor name 'Zed'; the level offers: Alpha"));
	CHECK_EQ(Resolve("class", "Zed", actors).Error, std::string("no actor in the level has the class name 'Zed'; the level offers: Pawn"));
	CHECK_EQ(Resolve("bind", "Zed", actors, true).Error, std::string("no actor in the level has the BindName 'Zed'; the level offers: PaulDenton"));
}

TEST(HarnessResolverOffersNoneForALevelWithNoActors)
{
	Actors actors;
	CHECK_EQ(Resolve("name", "Zed", actors).Error, std::string("no actor in the level has the actor name 'Zed'; the level offers: none"));
}

TEST(HarnessResolverOffersEachValueOnceInItsFirstSpellingAndNeverAnEmptyOne)
{
	Actors actors;
	actors.Add("Alpha");
	actors.Add("ALPHA");
	actors.Add("");
	actors.Add("Beta");

	CHECK_EQ(Resolve("name", "Zed", actors).Error, std::string("no actor in the level has the actor name 'Zed'; the level offers: Alpha, Beta"));
}

TEST(HarnessResolverOffersTwentyValuesThenCountsTheRest)
{
	Actors actors;
	AddNumbered(actors, 25);

	CHECK_EQ(Resolve("name", "Zed", actors).Error,
		std::string("no actor in the level has the actor name 'Zed'; the level offers: "
			"Actor00, Actor01, Actor02, Actor03, Actor04, Actor05, Actor06, Actor07, Actor08, Actor09, "
			"Actor10, Actor11, Actor12, Actor13, Actor14, Actor15, Actor16, Actor17, Actor18, Actor19, ... (5 more)"));
}

TEST(HarnessResolverCountsOnlyTheValuesBeyondTwenty)
{
	Actors twenty;
	AddNumbered(twenty, 20);
	std::string error = Resolve("name", "Zed", twenty).Error;
	CHECK_MSG(error.find("more") == std::string::npos, "twenty values were cut: " + error);
	CHECK_MSG(error.find("Actor19") != std::string::npos, "the twentieth value is missing: " + error);

	Actors twentyOne;
	AddNumbered(twentyOne, 21);
	error = Resolve("name", "Zed", twentyOne).Error;
	CHECK_MSG(error.find("Actor19, ... (1 more)") != std::string::npos, "the cut is not at twenty: " + error);
	CHECK_MSG(error.find("Actor20") == std::string::npos, "the twenty-first value is offered: " + error);
}

TEST(HarnessResolverDoesNotCountADeletedActorAmongTheValuesOffered)
{
	Actors actors;
	for (int i = 0; i < 5; i++)
		actors.AddDeleted(std::string("Gone") + std::to_string(i));
	AddNumbered(actors, 20);

	std::string error = Resolve("name", "Zed", actors).Error;
	CHECK_MSG(error.find("more") == std::string::npos, "a deleted actor was counted: " + error);
	CHECK_MSG(error.find("Gone") == std::string::npos, "a deleted actor was offered: " + error);
	CHECK_MSG(error.find("Actor19") != std::string::npos, "the live values were cut short: " + error);
}
