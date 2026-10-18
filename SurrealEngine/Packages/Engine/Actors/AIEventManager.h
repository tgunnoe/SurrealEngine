#pragma once

#include "Package/NameString.h"
#include "Math/vec.h"
#include "Packages/Engine/Actors/UActor.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

// What an actor asked to hear about through AISetEventCallback: the script function to call when the
// event fires, an optional function that scores a source, and which sight checks apply to it.
struct AIEventRegistration
{
	NameString Event;
	NameString Callback;
	NameString ScoreCallback;
	bool bCheckVisibility = false;
	bool bCheckDir = false;
	bool bCheckCylinder = false;
	bool bCheckLOS = false;
};

// The XAIParams a callback receives, before it is converted to the script struct.
struct AIEventParams
{
	UActor* bestActor = nullptr;
	float score = 0.0f;
	float visibility = 0.0f;
	float volume = 0.0f;
	float smell = 0.0f;
};

// Everything the manager needs to know about actors. The manager treats UActor* as an opaque key and
// calls nothing on it, so the engine supplies these and a test can supply plain tables instead.
struct AIEventHooks
{
	// Calls the listener's script callback. False means the listener's class has no such function.
	std::function<bool(UActor* listener, const NameString& callback, const NameString& event, EAIEventState state, const AIEventParams& params)> Deliver;
	// Runs the registration's score function on a raw score and returns the score to use.
	std::function<float(UActor* listener, const NameString& scoreCallback, UActor* sender, float rawScore)> Score;
	// Whether the listener passes the sight checks its registration asks for against the sender.
	std::function<bool(UActor* listener, UActor* sender, const AIEventRegistration& registration)> Visible;
	std::function<vec3(UActor* actor)> Position;
	// False once the actor is being or has been destroyed.
	std::function<bool(UActor* actor)> Alive;
	// Game seconds.
	std::function<double()> Now;
};

// Deus Ex routes AI events (noise, distress, alarms, carcasses and so on) through one manager per level.
// LevelInfo.InitEventManager creates it empty when the level starts; the event natives fill it.
//
// The original engine's cadence, best-sender rule and ChangeBest trigger cannot be recovered from the
// scripts, which only show how callbacks are registered and what they do with the parameters. The rules
// below are this engine's own.
//
// A listener registers one callback per event name; registering again replaces it. Emitters either send
// a one-shot event, delivered at once as Begin, or start a persistent one and end it later. For a
// persistent event each listener has a stream of states per event name, however many senders there are:
//
//   Begin       the listener starts hearing the name
//   Pulse       about once a second while it still hears it
//   ChangeBest  the best sender is not the one it last heard
//   End         it hears nothing of that name any more
//
// A sender is heard when the listener is inside its radius (strictly; no radius means anywhere), passes
// the sight checks of its registration, and the score is above zero. The score is the value, falling
// linearly to zero at the radius, then replaced by the registration's score function if it has one. The
// best sender is the highest score; an earlier-started event wins a tie. A listener never hears itself.
class AIEventManager
{
public:
	AIEventHooks Hooks;

	// Registers or replaces the (listener, event) callback. Replacing with the same callback, score
	// function and checks keeps what the listener is hearing; any change makes it start afresh.
	void SetCallback(UActor* listener, const AIEventRegistration& registration);
	void ClearCallback(UActor* listener, const NameString& event);

	// A radius of zero or less means the event carries anywhere.
	void SendEvent(UActor* sender, const NameString& event, EAIEventType type, float value, float radius);
	void StartEvent(UActor* sender, const NameString& event, EAIEventType type, float value, float radius);
	void EndEvent(UActor* sender, const NameString& event, EAIEventType type);

	// Called every tick with game seconds; acts at most once a second.
	void Pulse(double now);

	// Drops everything that involves the actor, ending what its events made listeners hear.
	void Forget(UActor* actor);

private:
	// One (listener, event name) registration and what that listener is hearing under it.
	struct Listener
	{
		uint64_t Id = 0;
		UActor* Actor = nullptr;
		AIEventRegistration Registration;
		bool Hearing = false;
		// Who the listener last heard as best: never an actor that has been forgotten.
		UActor* Best = nullptr;
	};

	// A persistent event a sender has started and not ended.
	struct Record
	{
		UActor* Sender = nullptr;
		NameString Event;
		EAIEventType Type = EAIEventType::EAITYPE_Audio;
		float Value = 1.0f;
		float Radius = 0.0f;
		double StartedAt = 0.0;
	};

	struct Candidate
	{
		Record Source;
		float Score = 0.0f;
	};

	// How much of the Begin, Pulse, ChangeBest, End machine a caller wants run for a listener.
	enum class SettleMode { Full, BeginOnly, EndOnly };

	// Listeners and records are copied before any hook runs: hooks run script, and script may register,
	// clear, send, start, end or destroy while the manager is in the middle of walking them.
	std::optional<Candidate> Consider(const Listener& listener, const Record& record);
	std::optional<Candidate> BestFor(Listener listener);
	void Settle(uint64_t id, SettleMode mode);
	void Notify(uint64_t id, EAIEventState state, const AIEventParams& params);
	void ForgetDead();
	std::vector<uint64_t> ListenerIds(const NameString& event) const;
	Listener* Find(uint64_t id);

	// In registration order, which is the order callbacks are called in.
	std::vector<Listener> Listeners;
	std::vector<Record> Records;
	uint64_t NextId = 1;
	double LastPulse = 0.0;
};
