#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UObject.h"
#include "Packages/Engine/Actors/UActor.h"
#include "Packages/ConSys/UConItem.h"
#include "Packages/ConSys/UConversation.h"
#include "Packages/ConSys/UConversationList.h"
#include "Packages/ConSys/UConversationMissionList.h"
#include "Packages/ConSys/Events/UConEvent.h"
#include "Packages/ConSys/Events/UConEventSpeech.h"
#include "Packages/ConSys/Events/UConEventTransferObject.h"
#include "Packages/ConSys/Events/UConEventMoveCamera.h"
#include "Packages/ConSys/Events/UConEventAnimation.h"
#include "Packages/ConSys/Events/UConEventTrade.h"
#include <cstring>

// The conversation events of a persistent package keep pointing at the actors of a level that was
// left during a conversation, and one such actor keeps its whole level alive. These pin which slots
// the unload clears and which conversations it walks. A Package cannot be built without a
// PackageManager, and the selection only compares the pointer, so distinct addresses stand in for
// packages; no collection runs here, so the stand-ins are never marked. That the level is then
// freed is shown by running a map.

namespace
{
	char PackageSlots[2];
	Package* const PackageA = reinterpret_cast<Package*>(&PackageSlots[0]);
	Package* const PackageB = reinterpret_cast<Package*>(&PackageSlots[1]);

	constexpr size_t SlotSize = sizeof(void*);

	PropertyDataOffset Slot(size_t index)
	{
		PropertyDataOffset offset;
		offset.DataOffset = index * SlotSize;
		return offset;
	}

	// Every object is one four-slot block. Slot 0 is the property a type reads first (the event
	// type, a list head, the conversation object), slot 1 the link to the next one, slots 2 and 3
	// the actors of an event.
	class Layout
	{
	public:
		Layout()
		{
			cls = GC::Alloc<UClass>("ConversationBlock", nullptr, ObjectFlags::NoFlags);
			cls->StructSize = 4 * SlotSize;
			cls->StructAlignment = SlotSize;
			cls->PropertyData.Init(cls);

			PropOffsets_ConEvent.eventType = Slot(0);
			PropOffsets_ConEvent.nextEvent = Slot(1);
			PropOffsets_ConEventSpeech.speaker = Slot(2);
			PropOffsets_ConEventSpeech.speakingTo = Slot(3);
			PropOffsets_ConEventTransferObject.fromActor = Slot(2);
			PropOffsets_ConEventTransferObject.toActor = Slot(3);
			PropOffsets_ConEventMoveCamera.cameraActor = Slot(2);
			PropOffsets_ConEventAnimation.eventOwner = Slot(2);
			PropOffsets_ConEventTrade.eventOwner = Slot(2);
			PropOffsets_Conversation.eventList = Slot(0);
			PropOffsets_ConversationMissionList.missions = Slot(0);
			PropOffsets_ConversationList.conversations = Slot(0);
			PropOffsets_ConItem.ConObject = Slot(0);
			PropOffsets_ConItem.Next = Slot(1);
		}

		template<typename T>
		T* Make(const char* name)
		{
			T* obj = GC::Alloc<T>(name, cls, ObjectFlags::NoFlags);
			obj->PropertyData.Init(cls);
			std::memset(obj->PropertyData.Data, 0, obj->PropertyData.Size);
			return obj;
		}

		UActor* Actor(Package* package)
		{
			UActor* actor = Make<UActor>("Actor");
			actor->package = package;
			return actor;
		}

		template<typename T>
		T* Event(EEventType type)
		{
			T* event = Make<T>("Event");
			event->eventType() = static_cast<uint8_t>(type);
			return event;
		}

	private:
		UClass* cls = nullptr;
	};

	// One event of each of the five types that carry actors, then a speech whose two actors are
	// in different packages, so a clear that tests the pair rather than each slot shows.
	struct Conversation
	{
		UConversation* con = nullptr;
		UConEventSpeech* speech = nullptr;
		UConEventTransferObject* transfer = nullptr;
		UConEventMoveCamera* camera = nullptr;
		UConEventAnimation* animation = nullptr;
		UConEventTrade* trade = nullptr;
		UConEventSpeech* mixed = nullptr;

		UActor* slots[7] = {};
	};

	Conversation Build(Layout& layout, Package* bound, Package* mixedSpeaker, Package* mixedSpeakingTo)
	{
		Conversation c;
		c.con = layout.Make<UConversation>("Conversation");
		c.speech = layout.Event<UConEventSpeech>(EEventType::Speech);
		c.transfer = layout.Event<UConEventTransferObject>(EEventType::TransferObject);
		c.camera = layout.Event<UConEventMoveCamera>(EEventType::MoveCamera);
		c.animation = layout.Event<UConEventAnimation>(EEventType::Animation);
		c.trade = layout.Event<UConEventTrade>(EEventType::Trade);
		c.mixed = layout.Event<UConEventSpeech>(EEventType::Speech);

		c.con->eventList() = c.speech;
		c.speech->nextEvent() = c.transfer;
		c.transfer->nextEvent() = c.camera;
		c.camera->nextEvent() = c.animation;
		c.animation->nextEvent() = c.trade;
		c.trade->nextEvent() = c.mixed;

		for (UActor*& actor : c.slots)
			actor = layout.Actor(bound);
		c.speech->speaker() = c.slots[0];
		c.speech->speakingTo() = c.slots[1];
		c.transfer->fromActor() = c.slots[2];
		c.transfer->toActor() = c.slots[3];
		c.camera->cameraActor() = c.slots[4];
		c.animation->eventOwner() = c.slots[5];
		c.trade->eventOwner() = c.slots[6];

		c.mixed->speaker() = layout.Actor(mixedSpeaker);
		c.mixed->speakingTo() = layout.Actor(mixedSpeakingTo);
		return c;
	}

	std::vector<UActor*> BoundActors(const Conversation& c)
	{
		return { c.speech->speaker(), c.speech->speakingTo(), c.transfer->fromActor(), c.transfer->toActor(),
			c.camera->cameraActor(), c.animation->eventOwner(), c.trade->eventOwner() };
	}

	// The chain must survive a clear: only the actor slots change.
	void CheckChainIntact(const Conversation& c)
	{
		CHECK(c.con->eventList() == c.speech);
		CHECK(c.speech->nextEvent() == c.transfer);
		CHECK(c.transfer->nextEvent() == c.camera);
		CHECK(c.camera->nextEvent() == c.animation);
		CHECK(c.animation->nextEvent() == c.trade);
		CHECK(c.trade->nextEvent() == c.mixed);
		CHECK(c.mixed->nextEvent() == nullptr);
		CHECK_EQ((int)c.speech->eventType(), (int)EEventType::Speech);
		CHECK_EQ((int)c.transfer->eventType(), (int)EEventType::TransferObject);
		CHECK_EQ((int)c.camera->eventType(), (int)EEventType::MoveCamera);
		CHECK_EQ((int)c.animation->eventType(), (int)EEventType::Animation);
		CHECK_EQ((int)c.trade->eventType(), (int)EEventType::Trade);
		CHECK_EQ((int)c.mixed->eventType(), (int)EEventType::Speech);
	}

	UConItem* Item(Layout& layout, UConObject* object, UConItem* next)
	{
		UConItem* item = layout.Make<UConItem>("Item");
		item->ConObject() = object;
		item->Next() = next;
		return item;
	}
}

TEST(UnbindClearsBoundActorsOfTheReleasedPackage)
{
	Layout layout;
	Conversation c = Build(layout, PackageA, PackageA, PackageB);
	UActor* keptListener = c.mixed->speakingTo();

	c.con->ClearBindEventsInto(PackageA);

	for (UActor* actor : BoundActors(c))
		CHECK(actor == nullptr);
	CHECK(c.mixed->speaker() == nullptr);
	CHECK(c.mixed->speakingTo() == keptListener);
	CheckChainIntact(c);
}

TEST(UnbindKeepsBoundActorsOfAnotherPackage)
{
	Layout layout;
	Conversation c = Build(layout, PackageB, PackageB, PackageB);
	std::vector<UActor*> before = BoundActors(c);
	UActor* mixedSpeaker = c.mixed->speaker();
	UActor* mixedSpeakingTo = c.mixed->speakingTo();

	c.con->ClearBindEventsInto(PackageA);

	CHECK(BoundActors(c) == before);
	CHECK(c.mixed->speaker() == mixedSpeaker);
	CHECK(c.mixed->speakingTo() == mixedSpeakingTo);
	CheckChainIntact(c);
}

TEST(UnbindLeavesUnboundSlotsNull)
{
	// Most events of a conversation that is not playing hold no actor, so the clear reads null slots
	// all the time and must not look through them.
	Layout layout;
	Conversation c = Build(layout, PackageA, PackageA, PackageB);
	UActor* keptListener = c.mixed->speakingTo();
	c.speech->speaker() = nullptr;
	c.transfer->toActor() = nullptr;
	c.trade->eventOwner() = nullptr;
	c.mixed->speaker() = nullptr;

	c.con->ClearBindEventsInto(PackageA);

	for (UActor* actor : BoundActors(c))
		CHECK(actor == nullptr);
	CHECK(c.mixed->speaker() == nullptr);
	CHECK(c.mixed->speakingTo() == keptListener);
	CheckChainIntact(c);
}

TEST(UnbindWalksEveryConversationOfEveryMission)
{
	Layout layout;
	Conversation inA[4];
	for (Conversation& c : inA)
		c = Build(layout, PackageA, PackageA, PackageA);
	Conversation inB = Build(layout, PackageB, PackageB, PackageB);
	std::vector<UActor*> keptBefore = BoundActors(inB);

	// Two missions of two conversations; the second mission also holds a fifth, bound elsewhere.
	UConversationList* first = layout.Make<UConversationList>("First");
	UConversationList* second = layout.Make<UConversationList>("Second");
	first->conversations() = Item(layout, inA[0].con, Item(layout, inA[1].con, nullptr));
	second->conversations() = Item(layout, inA[2].con, Item(layout, inA[3].con, Item(layout, inB.con, nullptr)));

	// A mission item that is not a conversation list sits between the two missions.
	UConObject* notAList = layout.Make<UConObject>("NotAList");
	UConversationMissionList* missions = layout.Make<UConversationMissionList>("Missions");
	missions->missions() = Item(layout, first, Item(layout, notAList, Item(layout, second, nullptr)));

	UConversation::ClearMissionBindsInto(nullptr, PackageA);
	UConversation::ClearMissionBindsInto(missions, PackageA);

	for (Conversation& c : inA)
	{
		for (UActor* actor : BoundActors(c))
			CHECK(actor == nullptr);
		CHECK(c.mixed->speaker() == nullptr);
		CHECK(c.mixed->speakingTo() == nullptr);
	}
	CHECK(BoundActors(inB) == keptBefore);
}

TEST(ClearBindEventsStillNullsEveryBoundActor)
{
	Layout layout;
	Conversation inA = Build(layout, PackageA, PackageA, PackageB);
	Conversation inB = Build(layout, PackageB, PackageB, PackageB);
	Conversation unowned = Build(layout, nullptr, nullptr, nullptr);

	for (Conversation* c : { &inA, &inB, &unowned })
	{
		c->con->ClearBindEvents();

		for (UActor* actor : BoundActors(*c))
			CHECK(actor == nullptr);
		CHECK(c->mixed->speaker() == nullptr);
		CHECK(c->mixed->speakingTo() == nullptr);
		CheckChainIntact(*c);
	}
}
