#pragma once

#include "ExpressionValue.h"

class UObject;
class UFunction;

enum class EventName
{
	// Probe names:
	Spawned, Destroyed, GainedChild, LostChild,
	Probe4, Probe5, Trigger, UnTrigger,
	Timer, HitWall, Falling, Landed,
	ZoneChange, Touch, UnTouch, Bump,
	BeginState, EndState, BaseChange, Attach,
	Detach, ActorEntered, ActorLeaving, KillCredit,
	AnimEnd, EndedRotation, InterpolateEnd, EncroachingOn,
	EncroachedBy, FootZoneChange, HeadZoneChange, PainTimer,
	SpeechTimer, MayFall, Probe34, Die,
	Tick, PlayerTick, Expired, Probe39,
	SeePlayer, EnemyNotVisible, HearNoise, UpdateEyeHeight,
	SeeMonster, SeeFriend, SpecialHandling, BotDesireability,
	Probe48, Probe49, Probe50, Probe51,
	Probe52, Probe53, Probe54, Probe55,
	Probe56, Probe57, Probe58, Probe59,
	Probe60, Probe61, Probe62, All,

	// Other events:
	PlayerCalcView, Resolved, ResolveFailed, PreBeginPlay,
	BeginPlay, PostBeginPlay, SetInitialState, SpawnNotification,
	PostTouch, FellOutOfWorld, UpdateTactics, PlayerInput,
	Reset, PreRender, RenderOverlays, PostRender,
	NotifyLevelChange, InitGame, PreLogin, Login,
	Possess, TravelPreAccept, AcceptInventory, TravelPostAccept,
	PostLogin, KeyType, KeyEvent,

	MaxEventNameValue // Why isn't this part of C++ after 40+ years of people doing this in both C and C++?
};

// Why a script call is not made, when it is not. CallEvent and the call command ask for this one
// answer, so what the VM skips and what the command reports as skipped cannot drift apart.
enum class ScriptCallSkip
{
	None,          // The call is made
	EventDisabled, // The object's state ignores the event
	LevelNotBegun, // An actor whose level has not begun play
	Deleting       // An actor being deleted
};

// The decision itself. The actor flags count only for an actor. allowDeleted lets a call through to
// an actor being deleted, which UActor::Destroy needs for the Destroyed event it sends after it has
// marked the actor.
ScriptCallSkip ScriptCallSkipReason(bool eventEnabled, bool isActor, bool begunPlay, bool deleted, bool allowDeleted);

// The decision for a context, with its event state already looked up: reads the actor flags.
ScriptCallSkip SkipReasonFor(UObject* context, bool eventEnabled, bool allowDeleted);

// The same for an event named by a script function, with no allowance for an actor being deleted.
ScriptCallSkip SkipReasonFor(UObject* context, const NameString& name);

ExpressionValue CallEvent(UObject* Context, EventName name, Array<ExpressionValue> args = {});
ExpressionValue CallEvent(UObject* Context, const NameString& name, Array<ExpressionValue> args = {});

UFunction* FindEventFunction(UObject* Context, const NameString& name);

NameString ToNameString(EventName name);
bool NameStringToEventName(const NameString& name, EventName& eventName);
