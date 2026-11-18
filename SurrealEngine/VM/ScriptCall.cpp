
#include "Precomp.h"
#include "ScriptCall.h"
#include "Frame.h"
#include "Packages/Engine/Actors/UActor.h"
#include "Packages/Engine/Actors/Info/ULevelInfo.h"
#include "Packages/Engine/Resources/Level/ULevel.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include <unordered_map>

NameString ToNameString(EventName name)
{
	static const NameString names[] =
	{
		// Probe names:
		"Spawned",      "Destroyed",       "GainedChild",     "LostChild",
		"Probe4",       "Probe5",          "Trigger",         "UnTrigger",
		"Timer",        "HitWall",         "Falling",         "Landed",
		"ZoneChange",   "Touch",           "UnTouch",         "Bump",
		"BeginState",   "EndState",        "BaseChange",      "Attach",
		"Detach",       "ActorEntered",    "ActorLeaving",    "KillCredit",
		"AnimEnd",      "EndedRotation",   "InterpolateEnd",  "EncroachingOn",
		"EncroachedBy", "FootZoneChange",  "HeadZoneChange",  "PainTimer",
		"SpeechTimer",  "MayFall",         "Probe34",         "Die",
		"Tick",         "PlayerTick",      "Expired",         "Probe39",
		"SeePlayer",    "EnemyNotVisible", "HearNoise",       "UpdateEyeHeight",
		"SeeMonster",   "SeeFriend",       "SpecialHandling", "BotDesireability",
		"Probe48",      "Probe49",         "Probe50",         "Probe51",
		"Probe52",      "Probe53",         "Probe54",         "Probe55",
		"Probe56",      "Probe57",         "Probe58",         "Probe59",
		"Probe60",      "Probe61",         "Probe62",         "All",

		// Other events:
		"PlayerCalcView", "Resolved", "ResolveFailed", "PreBeginPlay",
		"BeginPlay", "PostBeginPlay", "SetInitialState", "SpawnNotification",
		"PostTouch", "FellOutOfWorld", "UpdateTactics", "PlayerInput",
		"Reset", "PreRender", "RenderOverlays", "PostRender",
		"NotifyLevelChange", "InitGame", "PreLogin", "Login",
		"Possess", "TravelPreAccept", "AcceptInventory", "TravelPostAccept",
		"PostLogin", "KeyType", "KeyEvent"
	};
	return names[(int)name];
}

// This is so nasty. Maybe we should just put the probe names at the top of the name index table?
static std::unordered_map<int, EventName> CreateLookup()
{
	std::unordered_map<int, EventName> lookup;
	for (int i = 0; i < (int)EventName::MaxEventNameValue; i++)
	{
		EventName name = (EventName)i;
		lookup[ToNameString(name).GetCompareIndex()] = name;
	}
	return lookup;
}

bool NameStringToEventName(const NameString& name, EventName& eventName)
{
	static std::unordered_map<int, EventName> lookup = CreateLookup();
	auto it = lookup.find(name.GetCompareIndex());
	if (it == lookup.end())
		return false;
	eventName = it->second;
	return true;
}

ScriptCallSkip ScriptCallSkipReason(bool eventEnabled, bool isActor, bool begunPlay, bool deleted, bool allowDeleted)
{
	if (!eventEnabled)
		return ScriptCallSkip::EventDisabled;

	if (isActor)
	{
		if (!begunPlay)
			return ScriptCallSkip::LevelNotBegun;
		if (deleted && !allowDeleted)
			return ScriptCallSkip::Deleting;
	}

	return ScriptCallSkip::None;
}

ScriptCallSkip SkipReasonFor(UObject* context, bool eventEnabled, bool allowDeleted)
{
	// A disabled event is decided before the context is looked at
	UActor* actor = eventEnabled ? UObject::TryCast<UActor>(context) : nullptr;
	return ScriptCallSkipReason(eventEnabled, actor != nullptr, actor && actor->Level()->bBegunPlay(), actor && actor->bDeleteMe(), allowDeleted);
}

ScriptCallSkip SkipReasonFor(UObject* context, const NameString& name)
{
	return SkipReasonFor(context, context->IsEventEnabled(name), false);
}

ExpressionValue CallEvent(UObject* Context, EventName eventname, Array<ExpressionValue> args)
{
	// UActor::Destroy marks an actor deleted before it sends Destroyed, so that is the one event an
	// actor being deleted still gets
	if (SkipReasonFor(Context, Context->IsEventEnabled(eventname), eventname == EventName::Destroyed) != ScriptCallSkip::None)
		return ExpressionValue::NothingValue();

	UFunction* func = FindEventFunction(Context, ToNameString(eventname));
	if (func)
		return Frame::Call(func, Context, { args.data(), args.size() });
	else
		return ExpressionValue::NothingValue();
}

ExpressionValue CallEvent(UObject* Context, const NameString& name, Array<ExpressionValue> args)
{
	if (SkipReasonFor(Context, name) != ScriptCallSkip::None)
		return ExpressionValue::NothingValue();

	UFunction* func = FindEventFunction(Context, name);
	if (func)
		return Frame::Call(func, Context, { args.data(), args.size() });
	else
		return ExpressionValue::NothingValue();
}

UFunction* FindEventFunction(UObject* Context, const NameString& name)
{
	// Search states first

	NameString stateName = Context->GetStateName();
	if (!stateName.IsNone())
	{
		for (UClass* cls = Context->Class; cls != nullptr; cls = static_cast<UClass*>(cls->BaseStruct))
		{
			UState* state = cls->GetState(stateName);
			if (state)
			{
				UFunction* func = state->GetFunction(name);
				if (func)
				{
					return func;
				}
			}
		}
	}

	// Search normal member functions next

	for (UClass* cls = Context->Class; cls != nullptr; cls = static_cast<UClass*>(cls->BaseStruct))
	{
		UFunction* func = cls->GetFunction(name);
		if (func)
		{
			return func;
		}
	}

	return nullptr;
}
