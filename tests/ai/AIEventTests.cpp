#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Engine/Actors/AIEventManager.h"
#include <cmath>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <string>
#include <utility>

// These cover the Deus Ex AI event manager: who hears an event, which states reach a listener and in
// what order. The manager never touches an actor, so actors here are opaque numbered keys and every
// function it asks of the engine is answered from the tables in World.

namespace
{
	constexpr EAIEventType Visual = EAIEventType::EAITYPE_Visual;
	constexpr EAIEventType Audio = EAIEventType::EAITYPE_Audio;
	constexpr EAIEventType Smell = EAIEventType::EAITYPE_Olifactory;

	constexpr uintptr_t ActorBase = 0x10000;
	constexpr uintptr_t ActorStride = 0x100;

	// Keys only: the manager must never dereference one, so these point at nothing.
	UActor* Actor(int number)
	{
		return reinterpret_cast<UActor*>(ActorBase + (uintptr_t)number * ActorStride);
	}

	int ActorNumber(UActor* actor)
	{
		return actor ? (int)(((uintptr_t)actor - ActorBase) / ActorStride) : -1;
	}

	const char* StateName(EAIEventState state)
	{
		switch (state)
		{
		case EAIEventState::EAISTATE_Begin: return "Begin";
		case EAIEventState::EAISTATE_End: return "End";
		case EAIEventState::EAISTATE_Pulse: return "Pulse";
		case EAIEventState::EAISTATE_ChangeBest: return "ChangeBest";
		}
		return "?";
	}

	bool Near(float a, float b, float eps = 1e-4f)
	{
		return std::fabs(a - b) <= eps;
	}

	AIEventRegistration Reg(const char* event, const char* callback, const char* scoreCallback = "")
	{
		AIEventRegistration registration;
		registration.Event = event;
		registration.Callback = callback;
		registration.ScoreCallback = scoreCallback;
		return registration;
	}

	struct Delivery
	{
		UActor* listener = nullptr;
		std::string callback;
		std::string event;
		EAIEventState state = EAIEventState::EAISTATE_Begin;
		AIEventParams params;
	};

	struct World
	{
		World()
		{
			manager.Hooks.Deliver = [this](UActor* listener, const NameString& callback, const NameString& event, EAIEventState state, const AIEventParams& params)
			{
				attempts.push_back(callback.ToString());
				if (missingCallbacks.count(callback.ToString()))
					return false;
				log.push_back({ listener, callback.ToString(), event.ToString(), state, params });
				if (afterDelivery)
					afterDelivery(log.back());
				return true;
			};
			manager.Hooks.Score = [this](UActor* listener, const NameString& scoreCallback, UActor* sender, float rawScore)
			{
				scoreInputs.push_back(rawScore);
				auto it = scorers.find(scoreCallback.ToString());
				return it != scorers.end() ? it->second(listener, sender, rawScore) : rawScore;
			};
			manager.Hooks.Visible = [this](UActor* listener, UActor* sender, const AIEventRegistration& registration)
			{
				registrationsChecked.push_back(registration);
				return blind.count({ listener, sender }) == 0;
			};
			manager.Hooks.Position = [this](UActor* actor) { return position.at(actor); };
			manager.Hooks.Alive = [this](UActor* actor) { return dead.count(actor) == 0; };
			manager.Hooks.Now = [this]() { return clock; };
		}

		World(const World&) = delete;
		World& operator=(const World&) = delete;

		void Place(UActor* actor, float x, float y = 0.0f, float z = 0.0f)
		{
			position[actor] = vec3(x, y, z);
		}

		void PulseAt(double now)
		{
			clock = now;
			manager.Pulse(now);
		}

		// The states one listener was sent, oldest first, each with the number of its best actor
		// ("-" for none): "Begin(2) Pulse(2) End(2)". Optionally only one event name.
		std::string Trace(UActor* listener, const char* event = nullptr) const
		{
			std::string out;
			for (const Delivery& d : log)
			{
				if (d.listener != listener || (event && d.event != event))
					continue;
				if (!out.empty())
					out += ' ';
				out += StateName(d.state);
				out += '(';
				out += d.params.bestActor ? std::to_string(ActorNumber(d.params.bestActor)) : "-";
				out += ')';
			}
			return out;
		}

		AIEventManager manager;
		std::map<UActor*, vec3> position;
		std::set<UActor*> dead;
		// (listener, sender) pairs that fail the sight checks.
		std::set<std::pair<UActor*, UActor*>> blind;
		std::map<std::string, std::function<float(UActor*, UActor*, float)>> scorers;
		// Callback names the listener's class does not define.
		std::set<std::string> missingCallbacks;
		std::function<void(const Delivery&)> afterDelivery;

		std::vector<Delivery> log;
		std::vector<std::string> attempts;
		std::vector<float> scoreInputs;
		std::vector<AIEventRegistration> registrationsChecked;
		double clock = 0.0;
	};
}

TEST(AIEventOneShotReachesRegisteredListener)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("LoudNoise", "HandleLoudNoise"));

	w.manager.SendEvent(sender, "LoudNoise", Audio, 2.0f, 0.0f);

	CHECK_EQ(w.Trace(listener), "Begin(2)");
	CHECK_EQ(w.log[0].callback, "HandleLoudNoise");
	CHECK_EQ(w.log[0].event, "LoudNoise");
	CHECK(Near(w.log[0].params.score, 2.0f));
	CHECK(Near(w.log[0].params.volume, 2.0f));
	CHECK(Near(w.log[0].params.visibility, 0.0f));
	CHECK(Near(w.log[0].params.smell, 0.0f));
}

TEST(AIEventParamsFollowEventType)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("Heard", "OnHeard"));

	// A radius of 400 at distance 100 makes the score 3/4 of the value, so the type fields must carry
	// the value and not the score.
	w.manager.SendEvent(sender, "Heard", Visual, 4.0f, 400.0f);
	w.manager.SendEvent(sender, "Heard", Audio, 4.0f, 400.0f);
	w.manager.SendEvent(sender, "Heard", Smell, 4.0f, 400.0f);

	CHECK_EQ(w.log.size(), (size_t)3);
	CHECK(Near(w.log[0].params.visibility, 4.0f) && Near(w.log[0].params.volume, 0.0f) && Near(w.log[0].params.smell, 0.0f));
	CHECK(Near(w.log[1].params.visibility, 0.0f) && Near(w.log[1].params.volume, 4.0f) && Near(w.log[1].params.smell, 0.0f));
	CHECK(Near(w.log[2].params.visibility, 0.0f) && Near(w.log[2].params.volume, 0.0f) && Near(w.log[2].params.smell, 4.0f));
	for (const Delivery& d : w.log)
		CHECK(Near(d.params.score, 3.0f));
}

TEST(AIEventOnlyTheRegisteredNameIsDelivered)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("LoudNoise", "HandleLoudNoise"));

	w.manager.SendEvent(sender, "Distress", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "");

	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
}

TEST(AIEventOneShotHonoursRangeSightAndSelf)
{
	World w;
	UActor* sender = Actor(1);
	UActor* near = Actor(2);
	UActor* far = Actor(3);
	UActor* blinded = Actor(4);
	UActor* otherName = Actor(5);
	w.Place(sender, 0);
	w.Place(near, 100);
	w.Place(far, 2000);
	w.Place(blinded, 100);
	w.Place(otherName, 100);
	w.blind.insert({ blinded, sender });
	// Registered in this order on purpose: delivery follows registration order.
	w.manager.SetCallback(near, Reg("LoudNoise", "OnNoise"));
	w.manager.SetCallback(sender, Reg("LoudNoise", "OnNoise"));
	w.manager.SetCallback(far, Reg("LoudNoise", "OnNoise"));
	w.manager.SetCallback(blinded, Reg("LoudNoise", "OnNoise"));
	w.manager.SetCallback(otherName, Reg("Distress", "OnDistress"));

	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 500.0f);

	CHECK_EQ(w.log.size(), (size_t)1);
	CHECK_EQ(w.Trace(near), "Begin(1)");
	CHECK_EQ(w.Trace(sender), "");
	CHECK_EQ(w.Trace(far), "");
	CHECK_EQ(w.Trace(blinded), "");
	CHECK_EQ(w.Trace(otherName), "");
}

TEST(AIEventOneShotDeliversInRegistrationOrder)
{
	World w;
	UActor* sender = Actor(9);
	w.Place(sender, 0);
	const int order[] = { 4, 2, 7, 3 };
	for (int number : order)
	{
		w.Place(Actor(number), 50);
		w.manager.SetCallback(Actor(number), Reg("Futz", "OnFutz"));
	}

	w.manager.SendEvent(sender, "Futz", Audio, 1.0f, 0.0f);

	CHECK_EQ(w.log.size(), (size_t)4);
	for (size_t i = 0; i < 4; i++)
		CHECK_EQ(ActorNumber(w.log[i].listener), order[i]);
}

TEST(AIEventOneShotLeavesNoHearingState)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("LoudNoise", "OnNoise"));

	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	w.PulseAt(5.0);

	// Two sends are two Begins; nothing is left to pulse or end.
	CHECK_EQ(w.Trace(listener), "Begin(2) Begin(2)");
}

TEST(AIEventRangeIsStrictAndScoreFallsOffLinearly)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.manager.SetCallback(listener, Reg("LoudNoise", "OnNoise"));

	w.Place(sender, 512);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 2.0f, 512.0f);
	CHECK_EQ(w.Trace(listener), "");

	w.Place(sender, 511);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 2.0f, 512.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
	CHECK(Near(w.log[0].params.score, 2.0f * (1.0f - 511.0f / 512.0f)));

	w.Place(sender, 25);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 2.0f, 100.0f);
	CHECK(Near(w.log.back().params.score, 1.5f));
}

TEST(AIEventNoRadiusMeansUnlimitedRange)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 1000000);
	w.manager.SetCallback(listener, Reg("LoudNoise", "OnNoise"));

	w.manager.SendEvent(sender, "LoudNoise", Audio, 3.0f, 0.0f);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 3.0f, -5.0f);

	CHECK_EQ(w.Trace(listener), "Begin(2) Begin(2)");
	CHECK(Near(w.log[0].params.score, 3.0f));
	CHECK(Near(w.log[1].params.score, 3.0f));
}

TEST(AIEventScoreCallbackVetoesAndReplaces)
{
	World w;
	UActor* listener = Actor(1);
	UActor* friendly = Actor(2);
	UActor* hostile = Actor(3);
	UActor* weird = Actor(4);
	w.Place(listener, 0);
	w.Place(friendly, 100);
	w.Place(hostile, 100);
	w.Place(weird, 100);
	w.scorers["NoiseScore"] = [&](UActor*, UActor* sender, float raw)
	{
		if (sender == friendly) return 0.0f;
		if (sender == weird) return -3.0f;
		return raw * 10.0f;
	};
	w.manager.SetCallback(listener, Reg("LoudNoise", "OnNoise", "NoiseScore"));

	w.manager.SendEvent(friendly, "LoudNoise", Audio, 1.0f, 0.0f);
	w.manager.SendEvent(weird, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.log.size(), (size_t)0);
	CHECK_EQ(w.scoreInputs.size(), (size_t)2);

	w.manager.SendEvent(hostile, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(3)");
	CHECK(Near(w.log[0].params.score, 10.0f));
	// The function sees the raw score, and the value reported alongside is still the emitter's.
	CHECK(Near(w.scoreInputs.back(), 1.0f));
	CHECK(Near(w.log[0].params.volume, 1.0f));
}

TEST(AIEventScoreAndSightAreOnlyAskedInsideTheRange)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.scorers["NoiseScore"] = [](UActor*, UActor*, float raw) { return raw; };
	AIEventRegistration registration = Reg("LoudNoise", "OnNoise", "NoiseScore");
	registration.bCheckLOS = true;
	w.manager.SetCallback(listener, registration);

	w.Place(sender, 900);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 500.0f);
	CHECK_EQ(w.registrationsChecked.size(), (size_t)0);
	CHECK_EQ(w.scoreInputs.size(), (size_t)0);

	// Inside the range but unseen: the sight check runs, the score function does not.
	w.Place(sender, 100);
	w.blind.insert({ listener, sender });
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 500.0f);
	CHECK_EQ(w.registrationsChecked.size(), (size_t)1);
	CHECK_EQ(w.scoreInputs.size(), (size_t)0);
	CHECK_EQ(w.log.size(), (size_t)0);

	w.blind.clear();
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 500.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
	CHECK_EQ(w.scoreInputs.size(), (size_t)1);
}

TEST(AIEventSightCheckReceivesTheRegistrationFlags)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	AIEventRegistration registration = Reg("Alarm", "OnAlarm");
	registration.bCheckVisibility = true;
	registration.bCheckCylinder = true;
	w.manager.SetCallback(listener, registration);

	w.manager.SendEvent(sender, "Alarm", Visual, 1.0f, 0.0f);

	CHECK_EQ(w.registrationsChecked.size(), (size_t)1);
	CHECK(w.registrationsChecked[0].bCheckVisibility);
	CHECK(w.registrationsChecked[0].bCheckCylinder);
	CHECK(!w.registrationsChecked[0].bCheckDir);
	CHECK(!w.registrationsChecked[0].bCheckLOS);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
}

TEST(AIEventRegisteringTwiceReplacesTheCallback)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("LoudNoise", "First"));
	w.manager.SetCallback(listener, Reg("LoudNoise", "Second"));

	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);

	// One registration per (listener, name): replaced, not appended.
	CHECK_EQ(w.log.size(), (size_t)1);
	CHECK_EQ(w.log[0].callback, "Second");
}

TEST(AIEventIdenticalReregistrationKeepsWhatTheListenerHears)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("Alarm", "OnAlarm", "AlarmScore"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");

	// UpdateReactionCallbacks runs again and again with the same arguments.
	w.manager.SetCallback(listener, Reg("Alarm", "OnAlarm", "AlarmScore"));
	w.manager.SetCallback(listener, Reg("Alarm", "OnAlarm", "AlarmScore"));
	w.PulseAt(1.0);

	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2)");
}

TEST(AIEventChangedReregistrationStartsAfresh)
{
	// Each change below is to a different part of the registration; all of them reset.
	for (int variant = 0; variant < 6; variant++)
	{
		World w;
		UActor* listener = Actor(1);
		UActor* sender = Actor(2);
		w.Place(listener, 0);
		w.Place(sender, 100);
		w.manager.SetCallback(listener, Reg("Alarm", "OnAlarm"));
		w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
		CHECK_EQ(w.Trace(listener), "Begin(2)");

		AIEventRegistration changed = Reg("Alarm", "OnAlarm");
		switch (variant)
		{
		case 0: changed.Callback = "OnOtherAlarm"; break;
		case 1: changed.ScoreCallback = "AlarmScore"; break;
		case 2: changed.bCheckVisibility = true; break;
		case 3: changed.bCheckDir = true; break;
		case 4: changed.bCheckCylinder = true; break;
		case 5: changed.bCheckLOS = true; break;
		}
		w.manager.SetCallback(listener, changed);
		w.PulseAt(1.0);

		// The reset delivers nothing by itself; the next pulse hears the alarm anew.
		CHECK_MSG(w.Trace(listener) == "Begin(2) Begin(2)", "variant " + std::to_string(variant) + ": " + w.Trace(listener));
		CHECK_EQ(w.log.back().callback, variant == 0 ? "OnOtherAlarm" : "OnAlarm");
	}
}

TEST(AIEventClearStopsDeliveryAndForgetsWhatWasHeard)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("Alarm", "OnAlarm"));
	w.manager.SetCallback(listener, Reg("LoudNoise", "OnNoise"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener, "Alarm"), "Begin(2)");

	w.manager.ClearCallback(listener, "Alarm");
	w.PulseAt(1.0);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);

	// No End for a cleared registration, no pulses afterwards, and the other name is untouched.
	CHECK_EQ(w.Trace(listener, "Alarm"), "Begin(2)");
	CHECK_EQ(w.Trace(listener, "LoudNoise"), "Begin(2)");

	// Registering again starts from nothing heard: the running alarm begins afresh.
	w.manager.SetCallback(listener, Reg("Alarm", "OnAlarm"));
	w.PulseAt(2.0);
	CHECK_EQ(w.Trace(listener, "Alarm"), "Begin(2) Begin(2)");
}

TEST(AIEventPersistentBeginsAtOnceAndPulsesOnTheClock)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));

	w.clock = 10.0;
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");

	w.PulseAt(10.5);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
	w.PulseAt(11.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2)");
	w.PulseAt(11.9);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2)");
	w.PulseAt(12.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2) Pulse(2)");
}

TEST(AIEventPulseTimerRunsFromTheFirstRecord)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));

	// Late in the game: the first pulse is a second after the record, not instantly.
	w.clock = 100.0;
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	w.PulseAt(100.5);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
	w.PulseAt(101.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2)");

	// With nothing active the timer idles; a new record starts it again from its own time.
	w.manager.EndEvent(sender, "Alarm", Audio);
	w.clock = 500.0;
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	w.PulseAt(500.5);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2) End(2) Begin(2)");
	w.PulseAt(501.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2) End(2) Begin(2) Pulse(2)");
}

TEST(AIEventPersistentEndsWhenTheSenderEndsIt)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);

	// A different type is a different record.
	w.manager.EndEvent(sender, "Alarm", Visual);
	CHECK_EQ(w.Trace(listener), "Begin(2)");

	w.manager.EndEvent(sender, "Alarm", Audio);
	CHECK_EQ(w.Trace(listener), "Begin(2) End(2)");

	w.PulseAt(5.0);
	w.manager.EndEvent(sender, "Alarm", Audio);
	CHECK_EQ(w.Trace(listener), "Begin(2) End(2)");
}

TEST(AIEventStartingTheSameRecordAgainReplacesIt)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));

	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	w.manager.StartEvent(sender, "Alarm", Audio, 5.0f, 0.0f);
	w.PulseAt(1.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2)");
	CHECK(Near(w.log.back().params.score, 5.0f));

	// One record, so one end.
	w.manager.EndEvent(sender, "Alarm", Audio);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2) End(2)");
}

TEST(AIEventPersistentEndsWhenTheSenderIsForgotten)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);

	w.manager.Forget(sender);
	CHECK_EQ(w.Trace(listener), "Begin(2) End(2)");

	w.PulseAt(5.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) End(2)");
}

TEST(AIEventForgottenListenerHearsNothingMore)
{
	World w;
	UActor* gone = Actor(1);
	UActor* kept = Actor(2);
	UActor* sender = Actor(3);
	w.Place(gone, 0);
	w.Place(kept, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(gone, Reg("Alarm", "AlarmHeard"));
	w.manager.SetCallback(gone, Reg("LoudNoise", "OnNoise"));
	w.manager.SetCallback(kept, Reg("Alarm", "AlarmHeard"));
	w.manager.SetCallback(kept, Reg("LoudNoise", "OnNoise"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);

	w.manager.Forget(gone);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	w.manager.EndEvent(sender, "Alarm", Audio);
	w.PulseAt(2.0);

	// No End for a listener that is gone; the one that stayed gets everything.
	CHECK_EQ(w.Trace(gone), "Begin(3)");
	CHECK_EQ(w.Trace(kept), "Begin(3) Begin(3) End(3)");
}

TEST(AIEventListenerRegisteringLateBeginsAtTheNextPulse)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);

	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	CHECK_EQ(w.Trace(listener), "");

	w.PulseAt(1.0);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
	w.PulseAt(2.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2)");
}

TEST(AIEventListenerOutOfRangeAtStartHearsOnceItIsInRange)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 1000);
	w.Place(sender, 0);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 500.0f);
	CHECK_EQ(w.Trace(listener), "");

	w.Place(listener, 100);
	w.PulseAt(1.0);
	CHECK_EQ(w.Trace(listener), "Begin(2)");

	// Walking out of range ends it at the next pulse, naming who it had been hearing.
	w.Place(listener, 1000);
	w.PulseAt(2.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) End(2)");

	w.Place(listener, 100);
	w.PulseAt(3.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) End(2) Begin(2)");
}

TEST(AIEventBetterSenderTakesOverAtTheNextPulse)
{
	World w;
	UActor* listener = Actor(1);
	UActor* first = Actor(2);
	UActor* better = Actor(3);
	w.Place(listener, 0);
	w.Place(first, 100);
	w.Place(better, 200);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(first, "Alarm", Audio, 1.0f, 0.0f);

	// The stream already begun does not begin again for a second sender.
	w.manager.StartEvent(better, "Alarm", Audio, 3.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");

	w.PulseAt(1.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) ChangeBest(3)");
	CHECK(Near(w.log.back().params.score, 3.0f));
	w.PulseAt(2.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) ChangeBest(3) Pulse(3)");
}

TEST(AIEventBestEndingWithAnotherRemainingChangesBest)
{
	World w;
	UActor* listener = Actor(1);
	UActor* weak = Actor(2);
	UActor* strong = Actor(3);
	w.Place(listener, 0);
	w.Place(weak, 100);
	w.Place(strong, 200);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(weak, "Alarm", Audio, 1.0f, 0.0f);
	w.manager.StartEvent(strong, "Alarm", Audio, 3.0f, 0.0f);
	w.PulseAt(1.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) ChangeBest(3)");

	// The listener still hears the alarm, so there is no End; the weaker sender takes over at the pulse.
	w.manager.EndEvent(strong, "Alarm", Audio);
	CHECK_EQ(w.Trace(listener), "Begin(2) ChangeBest(3)");
	w.PulseAt(2.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) ChangeBest(3) ChangeBest(2)");

	w.manager.EndEvent(weak, "Alarm", Audio);
	CHECK_EQ(w.Trace(listener), "Begin(2) ChangeBest(3) ChangeBest(2) End(2)");
}

TEST(AIEventForgettingTheBestSenderChangesBest)
{
	World w;
	UActor* listener = Actor(1);
	UActor* weak = Actor(2);
	UActor* strong = Actor(3);
	w.Place(listener, 0);
	w.Place(weak, 100);
	w.Place(strong, 200);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(weak, "Alarm", Audio, 1.0f, 0.0f);
	w.manager.StartEvent(strong, "Alarm", Audio, 3.0f, 0.0f);
	w.PulseAt(1.0);

	w.manager.Forget(strong);
	CHECK_EQ(w.Trace(listener), "Begin(2) ChangeBest(3)");
	w.PulseAt(2.0);

	// Never a pulse naming an actor that has been forgotten.
	CHECK_EQ(w.Trace(listener), "Begin(2) ChangeBest(3) ChangeBest(2)");
}

TEST(AIEventEarlierSenderWinsATie)
{
	World w;
	UActor* listener = Actor(1);
	UActor* earlier = Actor(2);
	UActor* later = Actor(3);
	w.Place(listener, 0);
	w.Place(earlier, 100);
	w.Place(later, 100);
	w.clock = 1.0;
	w.manager.StartEvent(earlier, "Alarm", Audio, 2.0f, 0.0f);
	w.clock = 2.0;
	w.manager.StartEvent(later, "Alarm", Audio, 2.0f, 0.0f);

	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.PulseAt(3.0);
	w.PulseAt(4.0);

	// The same sender stays best, so there is no ChangeBest flapping between equals.
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2)");
}

TEST(AIEventTwoSendersGiveOneDeliveryPerListenerPerPulse)
{
	World w;
	UActor* listener = Actor(1);
	UActor* a = Actor(2);
	UActor* b = Actor(3);
	UActor* c = Actor(4);
	w.Place(listener, 0);
	w.Place(a, 100);
	w.Place(b, 150);
	w.Place(c, 200);
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(a, "Alarm", Audio, 1.0f, 0.0f);
	w.manager.StartEvent(b, "Alarm", Audio, 1.0f, 0.0f);
	w.manager.StartEvent(c, "Alarm", Audio, 1.0f, 0.0f);

	for (int second = 1; second <= 3; second++)
	{
		size_t before = w.log.size();
		w.PulseAt(second);
		CHECK_EQ(w.log.size(), before + 1);
	}
	CHECK_EQ(w.Trace(listener), "Begin(2) Pulse(2) Pulse(2) Pulse(2)");
}

TEST(AIEventPersistentScoreCallbackPicksTheBestAmongTheScored)
{
	World w;
	UActor* listener = Actor(1);
	UActor* ally = Actor(2);
	UActor* enemy = Actor(3);
	w.Place(listener, 0);
	w.Place(ally, 100);
	w.Place(enemy, 100);
	w.scorers["AlarmScore"] = [&](UActor*, UActor* sender, float raw) { return sender == ally ? 0.0f : raw; };
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard", "AlarmScore"));

	// The ally's alarm is louder but vetoed, so it never becomes the best.
	w.manager.StartEvent(ally, "Alarm", Audio, 9.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "");
	w.manager.StartEvent(enemy, "Alarm", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(3)");
	w.PulseAt(1.0);
	CHECK_EQ(w.Trace(listener), "Begin(3) Pulse(3)");
}

TEST(AIEventDeliverFalseRemovesTheRegistration)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.missingCallbacks.insert("NoSuchFunction");
	w.manager.SetCallback(listener, Reg("LoudNoise", "NoSuchFunction"));

	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.attempts.size(), (size_t)1);

	// Gone after the first failure, not retried on every event.
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.attempts.size(), (size_t)1);

	// The same holds for a persistent event: one attempt at the start, none at the pulses.
	w.manager.SetCallback(listener, Reg("Alarm", "NoSuchFunction"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.attempts.size(), (size_t)2);
	w.PulseAt(1.0);
	w.PulseAt(2.0);
	CHECK_EQ(w.attempts.size(), (size_t)2);

	// A working callback registered afterwards is unaffected.
	w.manager.SetCallback(listener, Reg("LoudNoise", "OnNoise"));
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
}

TEST(AIEventDeadActorsAreForgottenWhenTouched)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	UActor* bystander = Actor(3);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.Place(bystander, 0);
	w.manager.SetCallback(listener, Reg("LoudNoise", "OnNoise"));
	w.manager.SetCallback(bystander, Reg("LoudNoise", "OnNoise"));

	// A listener that is destroyed without anyone saying so is dropped the next time the manager runs.
	w.dead.insert(listener);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	w.dead.clear();
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "");
	CHECK_EQ(w.Trace(bystander), "Begin(2) Begin(2)");

	// A dead sender's running event ends at the next pulse.
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
	w.dead.insert(sender);
	w.PulseAt(1.0);
	CHECK_EQ(w.Trace(listener), "Begin(2) End(2)");
}

TEST(AIEventDeadSenderEmitsNothing)
{
	World w;
	UActor* listener = Actor(1);
	UActor* sender = Actor(2);
	w.Place(listener, 0);
	w.Place(sender, 100);
	w.manager.SetCallback(listener, Reg("LoudNoise", "OnNoise"));
	w.manager.SetCallback(listener, Reg("Alarm", "AlarmHeard"));

	w.dead.insert(sender);
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);
	w.PulseAt(2.0);
	CHECK_EQ(w.Trace(listener), "");

	w.dead.clear();
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(listener), "Begin(2)");
}

TEST(AIEventCallbackMayClearAnotherRegistrationMidDelivery)
{
	World w;
	UActor* sender = Actor(1);
	UActor* first = Actor(2);
	UActor* second = Actor(3);
	UActor* third = Actor(4);
	w.Place(sender, 0);
	for (int number = 2; number <= 4; number++)
	{
		w.Place(Actor(number), 100);
		w.manager.SetCallback(Actor(number), Reg("LoudNoise", "OnNoise"));
	}
	w.afterDelivery = [&](const Delivery& d)
	{
		if (d.listener == first)
			w.manager.ClearCallback(second, "LoudNoise");
	};

	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);

	// The registration cleared during the loop is skipped; the one after it is still reached.
	CHECK_EQ(w.Trace(first), "Begin(1)");
	CHECK_EQ(w.Trace(second), "");
	CHECK_EQ(w.Trace(third), "Begin(1)");
}

TEST(AIEventCallbackMayRegisterAndSendWhileDelivering)
{
	World w;
	UActor* sender = Actor(1);
	UActor* first = Actor(2);
	UActor* late = Actor(30);
	w.Place(sender, 0);
	w.Place(first, 100);
	w.Place(late, 100);
	w.manager.SetCallback(first, Reg("LoudNoise", "OnNoise"));
	bool registered = false;
	w.afterDelivery = [&](const Delivery& d)
	{
		if (d.listener != first || registered)
			return;
		registered = true;
		// Enough registrations to move the manager's storage while it is walking it.
		for (int number = 10; number < 30; number++)
		{
			w.Place(Actor(number), 100);
			w.manager.SetCallback(Actor(number), Reg("LoudNoise", "OnNoise"));
		}
		w.manager.SetCallback(late, Reg("LoudNoise", "OnNoise"));
		w.manager.SendEvent(first, "Distress", Audio, 1.0f, 0.0f);
	};

	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);

	// Registered mid-loop: not part of this send, but part of the next.
	CHECK_EQ(w.Trace(late), "");
	CHECK_EQ(w.Trace(first), "Begin(1)");
	w.manager.SendEvent(sender, "LoudNoise", Audio, 1.0f, 0.0f);
	CHECK_EQ(w.Trace(late), "Begin(1)");
}

TEST(AIEventCallbackMayEndTheEventItIsHearing)
{
	World w;
	UActor* sender = Actor(1);
	UActor* first = Actor(2);
	UActor* second = Actor(3);
	w.Place(sender, 0);
	w.Place(first, 100);
	w.Place(second, 100);
	w.manager.SetCallback(first, Reg("Alarm", "AlarmHeard"));
	w.manager.SetCallback(second, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);

	w.afterDelivery = [&](const Delivery& d)
	{
		if (d.listener == first && d.state == EAIEventState::EAISTATE_Pulse)
			w.manager.EndEvent(sender, "Alarm", Audio);
	};
	w.PulseAt(1.0);

	// The first listener's callback ended the alarm during the pulse: it and the second one both get End
	// once, and the second does not also get a pulse for an alarm that no longer exists.
	CHECK_EQ(w.Trace(first), "Begin(1) Pulse(1) End(1)");
	CHECK_EQ(w.Trace(second), "Begin(1) End(1)");
	w.PulseAt(2.0);
	CHECK_EQ(w.log.size(), (size_t)5);
}

TEST(AIEventCallbackMayDestroyTheSender)
{
	World w;
	UActor* sender = Actor(1);
	UActor* first = Actor(2);
	UActor* second = Actor(3);
	w.Place(sender, 0);
	w.Place(first, 100);
	w.Place(second, 100);
	w.manager.SetCallback(first, Reg("Alarm", "AlarmHeard"));
	w.manager.SetCallback(second, Reg("Alarm", "AlarmHeard"));
	w.manager.StartEvent(sender, "Alarm", Audio, 1.0f, 0.0f);

	w.afterDelivery = [&](const Delivery& d)
	{
		if (d.listener == first && d.state == EAIEventState::EAISTATE_Pulse)
			w.manager.Forget(sender);
	};
	w.PulseAt(1.0);

	CHECK_EQ(w.Trace(first), "Begin(1) Pulse(1) End(1)");
	CHECK_EQ(w.Trace(second), "Begin(1) End(1)");
}

TEST(AIEventStateGrammarHoldsUnderRandomOperations)
{
	// Whatever is registered, started, ended, moved and forgotten, each listener's stream per name must
	// read Begin (Pulse | ChangeBest)* End, repeated, with the stream reset (silently) by clearing,
	// changing or forgetting its registration.
	const char* events[] = { "Alarm", "Carcass" };
	const char* callbacks[] = { "OnOne", "OnTwo" };
	constexpr int ActorCount = 6;
	size_t stateCounts[4] = {};

	for (unsigned seed = 1; seed <= 60; seed++)
	{
		std::mt19937 rng(seed);
		auto pick = [&](int count) { return (int)(rng() % (unsigned)count); };

		World w;
		for (int i = 0; i < ActorCount; i++)
			w.Place(Actor(i), (float)(i * 250));
		w.scorers["Veto"] = [&](UActor*, UActor* sender, float raw) { return (ActorNumber(sender) % 3 == 0) ? 0.0f : raw; };

		std::map<std::pair<UActor*, std::string>, std::pair<std::string, std::string>> registered; // oracle: callback and score function
		std::map<std::pair<UActor*, std::string>, bool> hearing;
		std::set<UActor*> forgotten;
		size_t audited = 0;
		std::string lastOp;

		auto audit = [&]()
		{
			std::map<std::pair<UActor*, std::string>, int> perOp;
			for (; audited < w.log.size(); audited++)
			{
				const Delivery& d = w.log[audited];
				stateCounts[(int)d.state]++;
				auto key = std::make_pair(d.listener, d.event);
				std::string what = "seed " + std::to_string(seed) + " after " + lastOp + ": " + StateName(d.state) + " to " +
					std::to_string(ActorNumber(d.listener)) + " for " + d.event + " best " + std::to_string(ActorNumber(d.params.bestActor));
				CHECK_MSG(registered.count(key) != 0, what + " (not registered)");
				CHECK_MSG(registered[key].first == d.callback, what + " (wrong callback)");
				CHECK_MSG(d.params.bestActor != d.listener, what + " (own event)");
				CHECK_MSG(++perOp[key] == 1, what + " (second delivery in one operation)");
				if (d.state != EAIEventState::EAISTATE_End)
					CHECK_MSG(d.params.bestActor && forgotten.count(d.params.bestActor) == 0, what + " (forgotten or no best)");
				bool& isHearing = hearing[key];
				switch (d.state)
				{
				case EAIEventState::EAISTATE_Begin:
					CHECK_MSG(!isHearing, what + " (Begin while hearing)");
					isHearing = true;
					break;
				case EAIEventState::EAISTATE_End:
					CHECK_MSG(isHearing, what + " (End while silent)");
					isHearing = false;
					break;
				default:
					CHECK_MSG(isHearing, what + " (state while silent)");
					break;
				}
			}
		};

		double now = 0.0;
		for (int step = 0; step < 400; step++)
		{
			UActor* actor = Actor(pick(ActorCount));
			const char* event = events[pick(2)];
			switch (pick(8))
			{
			case 0:
			{
				const std::string callback = callbacks[pick(2)];
				const std::string score = pick(2) ? "Veto" : "";
				auto key = std::make_pair(actor, std::string(event));
				auto previous = registered.find(key);
				// Same callback and score function keeps what the listener hears; anything else starts afresh.
				if (previous == registered.end() || previous->second != std::make_pair(callback, score))
					hearing[key] = false;
				registered[key] = { callback, score };
				w.manager.SetCallback(actor, Reg(event, callback.c_str(), score.c_str()));
				lastOp = "register";
				break;
			}
			case 1:
			{
				auto key = std::make_pair(actor, std::string(event));
				registered.erase(key);
				hearing.erase(key);
				w.manager.ClearCallback(actor, event);
				lastOp = "clear";
				break;
			}
			case 2:
			case 3:
				forgotten.erase(actor);
				w.manager.StartEvent(actor, event, pick(2) ? Audio : Visual, 1.0f + (float)pick(4), pick(2) ? 0.0f : 300.0f + (float)pick(900));
				lastOp = "start";
				break;
			case 4:
				w.manager.EndEvent(actor, event, Audio);
				w.manager.EndEvent(actor, event, Visual);
				lastOp = "end";
				break;
			case 5:
				for (auto it = registered.begin(); it != registered.end();)
					it = (it->first.first == actor) ? registered.erase(it) : std::next(it);
				for (auto it = hearing.begin(); it != hearing.end();)
					it = (it->first.first == actor) ? hearing.erase(it) : std::next(it);
				forgotten.insert(actor);
				w.manager.Forget(actor);
				lastOp = "forget";
				break;
			case 6:
				w.Place(actor, (float)pick(1500));
				lastOp = "move";
				break;
			default:
				now += 0.3 + 0.2 * pick(5);
				w.PulseAt(now);
				lastOp = "pulse";
				break;
			}
			audit();
		}
	}

	// A generator that never reaches a state would pass the grammar vacuously.
	for (int state = 0; state < 4; state++)
		CHECK_MSG(stateCounts[state] > 20, std::string("too few ") + StateName((EAIEventState)state) + " deliveries: " + std::to_string(stateCounts[state]));
}
