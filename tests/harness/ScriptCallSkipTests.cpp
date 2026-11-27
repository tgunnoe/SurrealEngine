#include "Precomp.h"
#include "TestRunner.h"
#include "VM/ScriptCall.h"
#include "Packages/Core/UClass.h"

// The decision whether a script call is made lives in one function, shared by the VM's two
// CallEvent overloads and the call command. These cover it as a pure function over its flags, and
// the wrapper for a context in the one case a test can reach: an object with no level, so that the
// begun-play and being-deleted branches of the wrapper, which read a real actor, are checked by
// reading the code and by running maps.

namespace
{
	const bool Both[] = { false, true };

	UObject* PlainObject()
	{
		UClass* cls = GC::Alloc<UClass>("TestClass", nullptr, ObjectFlags::NoFlags);
		return GC::Alloc<UObject>("TestObject", cls, ObjectFlags::NoFlags);
	}
}

TEST(HarnessSkipReasonIsEventDisabledWhateverElseHolds)
{
	for (bool isActor : Both)
		for (bool begun : Both)
			for (bool deleted : Both)
				for (bool allowDeleted : Both)
					CHECK(ScriptCallSkipReason(false, isActor, begun, deleted, allowDeleted) == ScriptCallSkip::EventDisabled);
}

TEST(HarnessSkipReasonIsLevelNotBegunForAnActorBeforePlay)
{
	// Before play even the Destroyed allowance does not reach the actor
	for (bool deleted : Both)
		for (bool allowDeleted : Both)
			CHECK(ScriptCallSkipReason(true, true, false, deleted, allowDeleted) == ScriptCallSkip::LevelNotBegun);
}

TEST(HarnessSkipReasonIsDeletingForAnActorBeingDeletedUnlessAllowed)
{
	CHECK(ScriptCallSkipReason(true, true, true, true, false) == ScriptCallSkip::Deleting);
	CHECK(ScriptCallSkipReason(true, true, true, true, true) == ScriptCallSkip::None);
}

TEST(HarnessSkipReasonMakesTheCallForALivingActorInAPlayingLevel)
{
	for (bool allowDeleted : Both)
		CHECK(ScriptCallSkipReason(true, true, true, false, allowDeleted) == ScriptCallSkip::None);
}

TEST(HarnessSkipReasonIgnoresTheActorFlagsOfAnObjectThatIsNoActor)
{
	for (bool begun : Both)
		for (bool deleted : Both)
			for (bool allowDeleted : Both)
				CHECK(ScriptCallSkipReason(true, false, begun, deleted, allowDeleted) == ScriptCallSkip::None);
}

TEST(HarnessSkipReasonHoldsTheRuleOverEveryCombination)
{
	for (bool enabled : Both)
		for (bool isActor : Both)
			for (bool begun : Both)
				for (bool deleted : Both)
					for (bool allowDeleted : Both)
					{
						const ScriptCallSkip reason = ScriptCallSkipReason(enabled, isActor, begun, deleted, allowDeleted);

						// The call is made exactly when nothing stops it
						const bool stopped = !enabled || (isActor && (!begun || (deleted && !allowDeleted)));
						CHECK_MSG((reason == ScriptCallSkip::None) == !stopped, "the call is made or skipped against the rule");

						// Making the allowance never turns a made call into a skipped one
						if (reason == ScriptCallSkip::None)
							CHECK(ScriptCallSkipReason(enabled, isActor, begun, deleted, true) == ScriptCallSkip::None);
						// and withdrawing it never turns a skipped call into a made one
						if (reason != ScriptCallSkip::None)
							CHECK(ScriptCallSkipReason(enabled, isActor, begun, deleted, false) != ScriptCallSkip::None);
					}
}

TEST(HarnessSkipReasonForADisabledEventOnAPlainObject)
{
	UObject* object = PlainObject();
	object->DisableEvent("Foo");

	for (bool allowDeleted : Both)
	{
		CHECK(SkipReasonFor(object, object->IsEventEnabled(NameString("Foo")), allowDeleted) == ScriptCallSkip::EventDisabled);
		CHECK(SkipReasonFor(object, object->IsEventEnabled(NameString("Bar")), allowDeleted) == ScriptCallSkip::None);
	}
}

TEST(HarnessSkipReasonForANamedEventReadsTheObjectsEventState)
{
	UObject* object = PlainObject();
	CHECK(SkipReasonFor(object, NameString("Foo")) == ScriptCallSkip::None);

	object->DisableEvent("Foo");
	CHECK(SkipReasonFor(object, NameString("Foo")) == ScriptCallSkip::EventDisabled);
	CHECK(SkipReasonFor(object, NameString("Bar")) == ScriptCallSkip::None);
}
