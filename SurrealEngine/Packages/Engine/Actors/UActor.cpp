
#include "Precomp.h"
#include "UActor.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include "Packages/DeusEx/UScriptedPawn.h"
#include "Packages/Engine/UViewport.h"
#include "Packages/Engine/Actors/AIEventManager.h"
#include "Packages/Engine/Actors/USpawnNotify.h"
#include "Packages/Engine/Actors/Info/ULevelInfo.h"
#include "Packages/Engine/Actors/Info/UGameInfo.h"
#include "Packages/Engine/Actors/Pawn/UPlayerPawn.h"
#include "Packages/Engine/Actors/Pawn/UPawn_AIMath.h"
#include "Packages/Engine/Resources/Level/ULevel.h"
#include "Packages/Engine/Resources/Level/UModel.h"
#include "Packages/Engine/Subsystems/USurrealAudioDevice.h"
#include "Utils/Logger.h"
#include "Engine.h"
#include "VM/ScriptCall.h"
#include "VM/Frame.h"
#include <cstddef>
#include <cstdio>
#include <set>

UActor* UActor::Spawn(UClass* SpawnClass, std::optional<UActor*> SpawnOwner, std::optional<NameString> SpawnTag, std::optional<vec3> SpawnLocation, std::optional<Rotator> SpawnRotation)
{
	if (!SpawnClass || SpawnClass->ClsFlags & ClassFlags::Abstract)
	{
		LogMessage("Could not spawn class: " + (SpawnClass ? SpawnClass->Name.ToString() : std::string("null")));
		return nullptr;
	}

	vec3 location = SpawnLocation ? *SpawnLocation : Location();
	Rotator rotation = SpawnRotation ? *SpawnRotation : Rotation();

	float radius = SpawnClass->GetDefaultObject<UActor>()->CollisionRadius();
	float height = SpawnClass->GetDefaultObject<UActor>()->CollisionHeight();
	bool bCollideWorld = SpawnClass->GetDefaultObject<UActor>()->bCollideWorld();
	bool bCollideWhenPlacing = SpawnClass->GetDefaultObject<UActor>()->bCollideWhenPlacing();
	if (bCollideWorld || bCollideWhenPlacing)
	{
		auto result = CheckLocation(location, radius, height, bCollideWorld || bCollideWhenPlacing);
		if (!result.first)
		{
			LogMessage("Could not find usable location when trying to spawn: " + SpawnClass->Name.ToString());
			return nullptr;
		}
		location = result.second;
	}

	// To do: package needs to be grabbed from outer, or the "transient package" if it is None, a virtual package for runtime objects
	// To do: find unique new name in the package
	static std::map<NameString, int> nextIndex;
	NameString name = SpawnClass->Name.ToString() + std::to_string(nextIndex[SpawnClass->Name]++);
	UActor* actor = UObject::Cast<UActor>(engine->LevelPackage->NewObject(name, UObject::Cast<UClass>(SpawnClass), ObjectFlags::Transient, true));

	actor->Outer() = XLevel()->Outer();
	actor->XLevel() = XLevel();
	actor->Level() = Level();
	actor->Tag() = (SpawnTag && !SpawnTag->IsNone()) ? *SpawnTag : SpawnClass->Name;
	actor->bTicked() = bTicked(); // To do: should it tick in the same world tick it was spawned in or wait until the next one?
	actor->Instigator() = Instigator();
	actor->Brush() = nullptr;
	actor->Location() = location;
	actor->OldLocation() = location;
	actor->Rotation() = rotation;
	actor->Region().Zone = actor->Level();
	actor->Index = (int)XLevel()->Actors.size();
	XLevel()->Actors.push_back(actor);
	XLevel()->Collision.AddToCollision(actor);

	actor->SetOwner(SpawnOwner.has_value() && SpawnOwner.value() ? *SpawnOwner : nullptr);

	if (Level()->bBegunPlay())
	{
		CallEvent(actor, EventName::Spawned);
		CallEvent(actor, EventName::PreBeginPlay);
		CallEvent(actor, EventName::BeginPlay);

		if (actor->bDeleteMe())
		{
			LogMessage("Object deleted itself during Spawn!");
			return nullptr;
		}

		// To do: we need to call EventName::EncroachingOn events here?

		actor->InitActorZone();

		CallEvent(actor, EventName::PostBeginPlay);
		CallEvent(actor, EventName::SetInitialState);
		if (engine->LaunchInfo.IsDeusEx())
			CallEvent(actor, "PostPostBeginPlay");

		actor->InitBase();

		if (engine->LaunchInfo.ue1Version >= 400)
		{
			static bool spawnNotificationLocked = false;
			if (!spawnNotificationLocked)
			{
				struct NotificationLockGuard
				{
					NotificationLockGuard() { spawnNotificationLocked = true; }
					~NotificationLockGuard() { spawnNotificationLocked = false; }
				} lockGuard;

				for (USpawnNotify* notifyObj = Level()->SpawnNotify(); notifyObj != nullptr; notifyObj = notifyObj->Next())
				{
					UClass* cls = notifyObj->ActorClass();
					if (cls && actor->IsA(cls->Name))
						actor = UObject::Cast<UGameInfo>(CallEvent(notifyObj, EventName::SpawnNotification, { ExpressionValue::ObjectValue(actor) }).ToObject());
				}
			}
		}
	}

	return actor;
}

bool UActor::Destroy()
{
	//engine->LogMessage("UActor.Destroy(" + Class->FriendlyName.ToString() + ")");

	if (bStatic() || bNoDelete())
		return false;
	if (bDeleteMe())
		return true;

	bDeleteMe() = true;

	// Run the current state's EndState so that whatever the state started (a screen tint, a display
	// change) is undone. The state itself is kept: script such as Razor2 calls a state function after
	// destroying its actor. The call is composed here instead of using CallEvent, because CallEvent
	// passes only Destroyed to an actor already marked deleted, so a plain CallEvent would do nothing.
	// Where it runs matters. It follows bDeleteMe = true, so an EndState that destroys the actor again
	// (UDamage's does) returns at once at the check above instead of destroying twice. It precedes
	// SetBase and the rest of the teardown, so the owner is still set (Destroyed clears it through the
	// inventory, and an EndState such as Invisibility's needs it), and so whatever the EndState
	// registers again, such as a collision entry through SetCollision or a sound, is removed by the
	// steps that follow instead of outliving the actor.
	if (!GetStateName().IsNone() && SkipReasonFor(this, IsEventEnabled(EventName::EndState), true) == ScriptCallSkip::None)
	{
		if (UFunction* endState = FindEventFunction(this, ToNameString(EventName::EndState)))
		{
			try
			{
				Frame::Call(endState, this, ArrayView<ExpressionValue>());
			}
			catch (const std::exception& e)
			{
				LogMessage("Script error: EndState of " + Name.ToString() + " threw while it was destroyed: " + e.what());
			}
			catch (...)
			{
				LogMessage("Script error: EndState of " + Name.ToString() + " threw an unknown error while it was destroyed");
			}
		}
	}

	SetBase(nullptr, true);

	engine->audiodev->ActorDestroyed(this);

	ULevel* level = XLevel();

	RemoveFromBspNode();
	level->Collision.RemoveFromCollision(this);

	CallEvent(this, EventName::Destroyed);

	// Deus Ex: whatever this actor was telling AI listeners, or listening to, ends with it.
	if (Level() && Level()->EventManager)
		Level()->EventManager->Forget(this);

	if (engine->LaunchInfo.IsUnrealTournament_469())
	{
		for (const auto actor : Touching_UT469())
			if (actor)
				UnTouch(actor);
	}
	else
	{
		for (const auto actor : Touching())
			if (actor)
				UnTouch(actor);
	}


	SetOwner(nullptr);

	while (!ChildActors.empty())
	{
		ChildActors.back()->SetOwner(nullptr);
	}
	while (!BasedActors.empty())
	{
		BasedActors.back()->SetBase(nullptr, true);
	}

	if (Index == -1)
		throw std::runtime_error("Actor index was never set!");
	level->Actors[Index] = nullptr;

	return true;
}

void UActor::Tick(float elapsed)
{
	if (engine->LaunchInfo.IsDeusEx())
	{
		DistanceFromPlayer() = length(engine->viewport->Actor()->Location() - Location());
	}

	TickAnimation(elapsed);
	if (engine->LaunchInfo.IsDeusEx())
		TickBlendAnimation(elapsed);

	if (Role() >= ROLE_SimulatedProxy && IsEventEnabled(EventName::Tick))
	{
		CallEvent(this, EventName::Tick, { ExpressionValue::FloatValue(elapsed) });
	}

	if (StateFrame)
	{
		LatentRunState latentState = StateFrame->GetLatentState();
		if (latentState == LatentRunState::Sleep)
		{
			SleepTimeLeft = std::max(SleepTimeLeft - elapsed, 0.0f);
			if (SleepTimeLeft == 0.0f)
				StateFrame->SetLatentState(LatentRunState::Continue);
		}
		else if (latentState == LatentRunState::FinishInterpolation)
		{
			if (!bInterpolating())
				StateFrame->SetLatentState(LatentRunState::Continue);
		}

		if (Role() >= ROLE_SimulatedProxy && StateFrame->GetLatentState() == LatentRunState::Continue)
		{
			StateFrame->Tick();
		}
	}

	TickPhysics(elapsed);

	if (TimerRate() > 0.0f) // Role() == ROLE_Authority && RemoteRole() == ROLE_AutonomousProxy
	{
		TimerCounter() += elapsed;
		while (TimerRate() > 0.0f && TimerCounter() > TimerRate())
		{
			TimerCounter() -= TimerRate();
			if (!bTimerLoop())
				TimerRate() = 0.0f;
			CallEvent(this, EventName::Timer);
		}
	}
}

bool UActor::Move(const vec3& delta)
{
	return TryMove(delta).Fraction == 1.0f;
}

bool UActor::MoveSmooth(const vec3& delta)
{
	CollisionHit hit = TryMoveSmooth(delta);
	return hit.Fraction != 1.0f;
}

void UActor::MakeNoise(float loudness)
{
	UPawn* noisePawn = UObject::Cast<UPawn>(Instigator());

	if (!noisePawn || Level()->NetMode() == NM_Client)
		return;

	float currentTime = Level()->TimeSeconds();
	vec3 delta1 = noisePawn->noise1spot() - Location();
	vec3 delta2 = noisePawn->noise2spot() - Location();
	if ((noisePawn->noise1time() > currentTime - 0.2f && dot(delta1, delta1) < 2500.0f && noisePawn->noise1loudness() >= 0.9f * loudness) ||
		(noisePawn->noise2time() > currentTime - 0.2f && dot(delta2, delta2) < 2500.0f && noisePawn->noise2loudness() >= 0.9f * loudness))
	{
		return;
	}

	if (noisePawn->noise1time() < currentTime - 0.18f)
	{
		noisePawn->noise1time() = currentTime;
		noisePawn->noise1spot() = Location();
		noisePawn->noise1loudness() = loudness;
	}
	else if (noisePawn->noise2time() < currentTime - 0.18f)
	{
		noisePawn->noise2time() = currentTime;
		noisePawn->noise2spot() = Location();
		noisePawn->noise2loudness() = loudness;
	}
	else if (dot(delta1, delta1) < 2500.0f)
	{
		noisePawn->noise1time() = currentTime;
		noisePawn->noise1spot() = Location();
		noisePawn->noise1loudness() = loudness;
	}
	else if (noisePawn->noise2loudness() <= loudness)
	{
		noisePawn->noise2time() = currentTime;
		noisePawn->noise2spot() = Location();
		noisePawn->noise2loudness() = loudness;
	}

	for (UPawn* pawn = Level()->PawnList(); pawn != nullptr; pawn = pawn->nextPawn())
	{
		if (pawn != noisePawn && pawn->CanHearNoise(this, loudness))
		{
			CallEvent(pawn, EventName::HearNoise, { ExpressionValue::FloatValue(loudness), ExpressionValue::ObjectValue(this) });
		}
	}
}

bool UActor::PlayerCanSeeMe()
{
	for (UPawn* pawn = Level()->PawnList(); pawn != nullptr; pawn = pawn->nextPawn())
	{
		if (pawn == this)
			continue;

		vec3 L = Location() - pawn->Location();
		float dist2 = dot(L, L);

		// Too far away
		if (dist2 > 500 * 500)
			continue;

		// Without behind view the pawn can only see in a 75 degree cone in front of them
		if (!pawn->bBehindView())
		{
			vec3 viewDirection = Coords::Rotation(pawn->ViewRotation()).XAxis;
			if (dot(viewDirection, L) < 0.2588190451f * dist2)
				continue;
		}

		// Try check for line of sight
		vec3 eyePos = pawn->Location();
		eyePos.z += pawn->BaseEyeHeight();
		if (pawn->FastTrace(Location(), eyePos))
			return true;
	}
	return false;
}

UTexture* UActor::CreateTextureFromScreenShot(UViewport* vport)
{
	LogUnimplemented("Actor.CreateTextureFromScreenShot");
	return nullptr;
}

UTexture* UActor::CreateTextureFromBMP(const std::string& name, const std::string& filename)
{
	LogUnimplemented("Actor.CreateTextureFromBMP");
	return nullptr;
}

bool UActor::SaveObjectAsFile(const std::string& dir, UObject* object)
{
	LogUnimplemented("Actor.SaveObjectAsFile");
	return false;
}

bool UActor::LoadObjectAsFile(const std::string& dir, UObject* object)
{
	LogUnimplemented("Actor.LoadObjectAsFile");
	return false;
}

bool UActor::SaveGameSaveInfo(const std::string& dir, UObject* object)
{
	LogUnimplemented("Actor.SaveGameSaveInfo");
	return false;
}

bool UActor::LoadGameSaveInfo(const std::string& dir, UObject* object)
{
	LogUnimplemented("Actor.LoadGameSaveInfo");
	return false;
}

bool UActor::IsOSVer2kOrXP()
{
	return true;
}

// Deus Ex AI events: the engine's half of AIEventManager, which knows nothing about actors itself.
namespace
{
	// A class that lacks a callback it registered would otherwise be reported at every event.
	void ReportAIEventCallbackOnce(UActor* listener, const NameString& function, const std::string& problem)
	{
		static std::set<std::string> reported;
		std::string key = listener->Class->Name.ToString() + "." + function.ToString();
		if (reported.insert(key).second)
			LogMessage("AI event callback " + key + ": " + problem);
	}

	// The script struct is read through the C++ XAIParams, so refuse a package whose struct is laid out differently.
	bool IsXAIParamsLayout(UStruct* type)
	{
		struct Field { const char* name; size_t offset; };
		static const Field fields[] = {
			{ "BestActor", offsetof(XAIParams, BestActor) }, { "Score", offsetof(XAIParams, Score) },
			{ "Visibility", offsetof(XAIParams, Visibility) }, { "Volume", offsetof(XAIParams, Volume) },
			{ "Smell", offsetof(XAIParams, Smell) } };

		if (type->StructSize != sizeof(XAIParams) || type->Properties.size() != std::size(fields))
			return false;
		for (const Field& field : fields)
		{
			bool found = false;
			for (UProperty* prop : type->Properties)
				found = found || (prop->Name == field.name && prop->DataOffset.DataOffset == field.offset);
			if (!found)
				return false;
		}
		return true;
	}

	bool DeliverAIEvent(UActor* listener, const NameString& callback, const NameString& event, EAIEventState state, const AIEventParams& params)
	{
		UFunction* func = FindEventFunction(listener, callback);
		if (!func)
		{
			ReportAIEventCallbackOnce(listener, callback, "the class has no such function, so its registration is removed");
			return false;
		}

		// Callbacks are (Name event, EAIEventState state, XAIParams params); the struct type comes from the function itself.
		UStructProperty* paramsProperty = nullptr;
		int parmIndex = 0;
		for (UProperty* prop : func->Properties)
		{
			if (!AllFlags(prop->PropFlags, PropertyFlags::Parm) || AllFlags(prop->PropFlags, PropertyFlags::ReturnParm))
				continue;
			if (parmIndex++ == 2)
			{
				if (prop->ValueType == ExpressionValueType::ValueStruct)
					paramsProperty = static_cast<UStructProperty*>(prop);
				break;
			}
		}
		if (!paramsProperty || !paramsProperty->Struct || !IsXAIParamsLayout(paramsProperty->Struct))
		{
			ReportAIEventCallbackOnce(listener, callback, "its third parameter is not an XAIParams, so its registration is removed");
			return false;
		}

		if (state != EAIEventState::EAISTATE_Pulse)
		{
			static const char* const stateNames[] = { "Begin", "End", "Pulse", "ChangeBest" };
			char score[32];
			snprintf(score, sizeof(score), "%.2f", params.score);
			LogMessage("AIEvent: " + (params.bestActor ? params.bestActor->Name.ToString() : std::string("None")) + " " + event.ToString() + " " +
				stateNames[(int)state] + " -> " + listener->Name.ToString() + " (" + listener->Class->Name.ToString() + ")." +
				callback.ToString() + " score=" + score);
		}

		// The struct is passed as a variable that lives in scriptParams. Variable() places a property at its
		// offset inside a frame, so give it the address of the frame that puts the parameter on scriptParams.
		XAIParams scriptParams = { params.bestActor, params.score, params.visibility, params.volume, params.smell };
		void* frame = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(&scriptParams) - paramsProperty->DataOffset.DataOffset);

		Array<ExpressionValue> args;
		args.push_back(ExpressionValue::NameValue(event));
		args.push_back(ExpressionValue::ByteValue((uint8_t)state));
		args.push_back(ExpressionValue::Variable(frame, paramsProperty));
		CallEvent(listener, callback, std::move(args));
		return true;
	}

	float ScoreAIEvent(UActor* listener, const NameString& scoreCallback, UActor* sender, float rawScore)
	{
		if (!FindEventFunction(listener, scoreCallback))
		{
			ReportAIEventCallbackOnce(listener, scoreCallback, "the class has no such score function, so scores are used as they are");
			return rawScore;
		}
		ExpressionValue result = CallEvent(listener, scoreCallback, { ExpressionValue::ObjectValue(listener), ExpressionValue::ObjectValue(sender), ExpressionValue::FloatValue(rawScore) });
		return result.GetType() == ExpressionValueType::Nothing ? rawScore : result.ToFloat();
	}

	bool AIEventVisible(UActor* listener, UActor* sender, const AIEventRegistration& registration)
	{
		UPawn* pawn = UObject::TryCast<UPawn>(listener);
		if (!pawn)
		{
			// Turrets and sensors have no eyes, so every sight flag comes down to a clear line between the two.
			bool anyCheck = registration.bCheckVisibility || registration.bCheckDir || registration.bCheckCylinder || registration.bCheckLOS;
			return !anyCheck || listener->FastTrace(sender->Location(), listener->Location());
		}

		if (registration.bCheckLOS && !pawn->LineOfSightTo(sender, true))
			return false;

		// A cylinder test is a visibility test of the whole body.
		if (registration.bCheckVisibility || registration.bCheckCylinder)
			return pawn->AICanSee(sender, 1.0f, true, registration.bCheckDir, registration.bCheckCylinder, true) > 0.0f;

		if (registration.bCheckDir)
		{
			vec3 eye = pawn->Location();
			eye.z += pawn->BaseEyeHeight();
			const Rotator facing = pawn->Rotation();
			const Rotator added = pawn->AIAddViewRotation();
			return AIMath::InViewCone(eye, facing.Pitch, facing.Yaw, added.Pitch, added.Yaw, sender->Location(), pawn->PeripheralVision());
		}
		return true;
	}

	// The manager of this actor's level, set up to ask the engine its questions, or null in a level whose
	// scripts never created one.
	AIEventManager* AIEventManagerFor(UActor* actor, const char* native)
	{
		ULevelInfo* level = actor->Level();
		if (!level || !level->EventManager)
		{
			static bool reported = false;
			if (!reported)
			{
				reported = true;
				LogMessage(std::string("Actor.") + native + " ignored: this level never created an AI event manager");
			}
			return nullptr;
		}

		AIEventManager* manager = level->EventManager.get();
		if (!manager->Hooks.Deliver)
		{
			manager->Hooks.Deliver = &DeliverAIEvent;
			manager->Hooks.Score = &ScoreAIEvent;
			manager->Hooks.Visible = &AIEventVisible;
			manager->Hooks.Position = [](UActor* a) { return a->Location(); };
			manager->Hooks.Alive = [](UActor* a) { return !a->bDeleteMe(); };
			manager->Hooks.Now = [level]() { return (double)level->TimeSeconds(); };
		}
		return manager;
	}
}

void UActor::AIClearEvent(const NameString& eventName)
{
	LogUnimplemented("Actor.AIClearEvent");
}

void UActor::AIClearEventCallback(const NameString& eventName)
{
	if (AIEventManager* manager = AIEventManagerFor(this, "AIClearEventCallback"))
		manager->ClearCallback(this, eventName);
}

void UActor::AIEndEvent(const NameString& eventName, uint8_t eventType)
{
	if (AIEventManager* manager = AIEventManagerFor(this, "AIEndEvent"))
		manager->EndEvent(this, eventName, (EAIEventType)eventType);
}

float UActor::AIGetLightLevel(const vec3& Location)
{
	LogUnimplemented("Actor.AIGetLightLevel");
	return 1.0f;
}

void UActor::AISendEvent(const NameString& eventName, uint8_t eventType, std::optional<float> Value, std::optional<float> Radius)
{
	if (AIEventManager* manager = AIEventManagerFor(this, "AISendEvent"))
		manager->SendEvent(this, eventName, (EAIEventType)eventType, Value.value_or(1.0f), Radius.value_or(0.0f));
}

void UActor::AISetEventCallback(const NameString& eventName, const NameString& callback, std::optional<NameString> scoreCallback, std::optional<bool> bCheckVisibility, std::optional<bool> bCheckDir, std::optional<bool> bCheckCylinder, std::optional<bool> bCheckLOS)
{
	AIEventManager* manager = AIEventManagerFor(this, "AISetEventCallback");
	if (!manager)
		return;

	AIEventRegistration registration;
	registration.Event = eventName;
	registration.Callback = callback;
	registration.ScoreCallback = scoreCallback.value_or(NameString());
	registration.bCheckVisibility = bCheckVisibility.value_or(false);
	registration.bCheckDir = bCheckDir.value_or(false);
	registration.bCheckCylinder = bCheckCylinder.value_or(false);
	registration.bCheckLOS = bCheckLOS.value_or(false);
	manager->SetCallback(this, registration);
}

void UActor::AIStartEvent(const NameString& eventName, uint8_t eventType, std::optional<float> Value, std::optional<float> Radius)
{
	if (AIEventManager* manager = AIEventManagerFor(this, "AIStartEvent"))
		manager->StartEvent(this, eventName, (EAIEventType)eventType, Value.value_or(1.0f), Radius.value_or(0.0f));
}

float UActor::AIVisibility(std::optional<bool> bIncludeVelocity)
{
	LogUnimplemented("Actor.AIVisibility");
	//LogMessage(Name.ToString() + ": AIVisibility");
	return 0.0f;
}
