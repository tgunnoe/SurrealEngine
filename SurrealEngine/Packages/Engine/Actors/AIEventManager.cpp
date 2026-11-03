#include "Precomp.h"
#include "AIEventManager.h"
#include <algorithm>

namespace
{
	AIEventParams ParamsFor(UActor* best, float score, EAIEventType type, float value)
	{
		AIEventParams params;
		params.bestActor = best;
		params.score = score;
		switch (type)
		{
		case EAIEventType::EAITYPE_Visual: params.visibility = value; break;
		case EAIEventType::EAITYPE_Audio: params.volume = value; break;
		case EAIEventType::EAITYPE_Olifactory: params.smell = value; break;
		}
		return params;
	}

	bool SameBehaviour(const AIEventRegistration& a, const AIEventRegistration& b)
	{
		return a.Callback == b.Callback && a.ScoreCallback == b.ScoreCallback &&
			a.bCheckVisibility == b.bCheckVisibility && a.bCheckDir == b.bCheckDir &&
			a.bCheckCylinder == b.bCheckCylinder && a.bCheckLOS == b.bCheckLOS;
	}
}

void AIEventManager::SetCallback(UActor* listener, const AIEventRegistration& registration)
{
	if (!listener)
		return;

	for (Listener& existing : Listeners)
	{
		if (existing.Actor != listener || !(existing.Registration.Event == registration.Event))
			continue;
		// UpdateReactionCallbacks registers the same things over and over; that must not make the
		// listener hear the same alarm begin again.
		if (!SameBehaviour(existing.Registration, registration))
		{
			existing.Hearing = false;
			existing.Best = nullptr;
		}
		existing.Registration = registration;
		return;
	}

	Listener added;
	added.Id = NextId++;
	added.Actor = listener;
	added.Registration = registration;
	Listeners.push_back(added);
}

void AIEventManager::ClearCallback(UActor* listener, const NameString& event)
{
	std::erase_if(Listeners, [&](const Listener& l) { return l.Actor == listener && l.Registration.Event == event; });
}

void AIEventManager::SendEvent(UActor* sender, const NameString& event, EAIEventType type, float value, float radius)
{
	if (!sender)
		return;
	if (!Hooks.Alive(sender))
	{
		Forget(sender);
		return;
	}
	ForgetDead();

	Record shot;
	shot.Sender = sender;
	shot.Event = event;
	shot.Type = type;
	shot.Value = value;
	shot.Radius = radius;
	shot.StartedAt = Hooks.Now();

	for (uint64_t id : ListenerIds(event))
	{
		const Listener* found = Find(id);
		if (!found) // cleared by a callback earlier in this send
			continue;
		Listener listener = *found;
		std::optional<Candidate> heard = Consider(listener, shot);
		if (heard)
			Notify(id, EAIEventState::EAISTATE_Begin, ParamsFor(sender, heard->Score, type, value));
	}
}

void AIEventManager::StartEvent(UActor* sender, const NameString& event, EAIEventType type, float value, float radius)
{
	if (!sender)
		return;
	if (!Hooks.Alive(sender))
	{
		Forget(sender);
		return;
	}
	ForgetDead();

	const double now = Hooks.Now();
	if (Records.empty())
		LastPulse = now;

	auto same = std::find_if(Records.begin(), Records.end(), [&](const Record& r) { return r.Sender == sender && r.Event == event && r.Type == type; });
	if (same != Records.end())
	{
		same->Value = value;
		same->Radius = radius;
	}
	else
	{
		Record record;
		record.Sender = sender;
		record.Event = event;
		record.Type = type;
		record.Value = value;
		record.Radius = radius;
		record.StartedAt = now;
		Records.push_back(record);
	}

	// Listeners already hearing this name hear the new sender at the next pulse; the others need not wait.
	for (uint64_t id : ListenerIds(event))
		Settle(id, SettleMode::BeginOnly);
}

void AIEventManager::EndEvent(UActor* sender, const NameString& event, EAIEventType type)
{
	if (!sender)
		return;
	ForgetDead();

	size_t removed = std::erase_if(Records, [&](const Record& r) { return r.Sender == sender && r.Event == event && r.Type == type; });
	if (removed == 0)
		return;

	// A remaining sender takes over at the next pulse; only a listener left with nothing hears End now.
	for (uint64_t id : ListenerIds(event))
		Settle(id, SettleMode::EndOnly);
}

void AIEventManager::Pulse(double now)
{
	if (Records.empty() || now < LastPulse + 1.0)
		return;
	LastPulse = now;
	ForgetDead();

	std::vector<uint64_t> ids;
	for (const Listener& l : Listeners)
	{
		bool active = std::any_of(Records.begin(), Records.end(), [&](const Record& r) { return r.Event == l.Registration.Event; });
		if (active)
			ids.push_back(l.Id);
	}
	for (uint64_t id : ids)
		Settle(id, SettleMode::Full);
}

void AIEventManager::Forget(UActor* actor)
{
	if (!actor)
		return;

	// A destroyed listener cannot be called, so its registrations go without an End.
	std::erase_if(Listeners, [&](const Listener& l) { return l.Actor == actor; });

	std::vector<NameString> ended;
	for (const Record& r : Records)
	{
		if (r.Sender == actor && std::find(ended.begin(), ended.end(), r.Event) == ended.end())
			ended.push_back(r.Event);
	}
	std::erase_if(Records, [&](const Record& r) { return r.Sender == actor; });

	std::vector<uint64_t> hearing;
	for (const Listener& l : Listeners)
	{
		if (l.Hearing && std::find(ended.begin(), ended.end(), l.Registration.Event) != ended.end())
			hearing.push_back(l.Id);
	}
	for (uint64_t id : hearing)
		Settle(id, SettleMode::EndOnly);

	// A listener that still hears something else under that name will name its new best at the next pulse.
	for (Listener& l : Listeners)
	{
		if (l.Best == actor)
			l.Best = nullptr;
	}
}

void AIEventManager::ForgetDead()
{
	std::vector<UActor*> dead;
	auto note = [&](UActor* actor)
	{
		if (!Hooks.Alive(actor) && std::find(dead.begin(), dead.end(), actor) == dead.end())
			dead.push_back(actor);
	};
	for (const Listener& l : Listeners)
		note(l.Actor);
	for (const Record& r : Records)
		note(r.Sender);
	for (UActor* actor : dead)
		Forget(actor);
}

std::optional<AIEventManager::Candidate> AIEventManager::Consider(const Listener& listener, const Record& record)
{
	if (record.Sender == listener.Actor)
		return std::nullopt;

	// Cheapest test first: sight and score functions run traces and script.
	const float distance = length(Hooks.Position(record.Sender) - Hooks.Position(listener.Actor));
	const bool limited = record.Radius > 0.0f;
	if (limited && !(distance < record.Radius))
		return std::nullopt;
	float score = limited ? record.Value * (1.0f - distance / record.Radius) : record.Value;

	if (!Hooks.Visible(listener.Actor, record.Sender, listener.Registration))
		return std::nullopt;
	if (!listener.Registration.ScoreCallback.IsNone())
		score = Hooks.Score(listener.Actor, listener.Registration.ScoreCallback, record.Sender, score);
	if (!(score > 0.0f))
		return std::nullopt;

	Candidate heard;
	heard.Source = record;
	heard.Score = score;
	return heard;
}

std::optional<AIEventManager::Candidate> AIEventManager::BestFor(Listener listener)
{
	std::vector<Record> relevant;
	for (const Record& r : Records)
	{
		if (r.Event == listener.Registration.Event)
			relevant.push_back(r);
	}

	// Scripts act on Begin and Pulse but not on ChangeBest, so on equal scores the sender already heard
	// stays best; only then does the earlier event win.
	UActor* held = listener.Hearing ? listener.Best : nullptr;
	auto outranks = [&](const Candidate& a, const Candidate& b)
	{
		if (a.Score != b.Score)
			return a.Score > b.Score;
		const bool aHeld = a.Source.Sender == held;
		if (aHeld != (b.Source.Sender == held))
			return aHeld;
		return a.Source.StartedAt < b.Source.StartedAt;
	};

	std::optional<Candidate> best;
	for (const Record& record : relevant)
	{
		std::optional<Candidate> heard = Consider(listener, record);
		if (heard && (!best || outranks(*heard, *best)))
			best = heard;
	}
	return best;
}

void AIEventManager::Settle(uint64_t id, SettleMode mode)
{
	const Listener* found = Find(id);
	if (!found)
		return;
	if ((mode == SettleMode::EndOnly && !found->Hearing) || (mode == SettleMode::BeginOnly && found->Hearing))
		return;

	std::optional<Candidate> best = BestFor(*found);

	// What the listener hears is read again after the hooks above, which may have changed it.
	Listener* listener = Find(id);
	if (!listener)
		return;

	if (!listener->Hearing)
	{
		if (!best || mode == SettleMode::EndOnly)
			return;
		listener->Hearing = true;
		listener->Best = best->Source.Sender;
		Notify(id, EAIEventState::EAISTATE_Begin, ParamsFor(best->Source.Sender, best->Score, best->Source.Type, best->Source.Value));
		return;
	}

	if (!best)
	{
		if (mode == SettleMode::BeginOnly)
			return;
		AIEventParams ended;
		ended.bestActor = listener->Best;
		listener->Hearing = false;
		listener->Best = nullptr;
		Notify(id, EAIEventState::EAISTATE_End, ended);
		return;
	}

	if (mode != SettleMode::Full)
		return;
	EAIEventState state = (best->Source.Sender == listener->Best) ? EAIEventState::EAISTATE_Pulse : EAIEventState::EAISTATE_ChangeBest;
	listener->Best = best->Source.Sender;
	Notify(id, state, ParamsFor(best->Source.Sender, best->Score, best->Source.Type, best->Source.Value));
}

void AIEventManager::Notify(uint64_t id, EAIEventState state, const AIEventParams& params)
{
	const Listener* found = Find(id);
	if (!found)
		return;
	UActor* actor = found->Actor;
	const NameString callback = found->Registration.Callback;
	const NameString event = found->Registration.Event;

	// The state above was settled before this call, so a callback that throws cannot make the same
	// Begin come round again at every pulse.
	if (!Hooks.Deliver(actor, callback, event, state, params))
		std::erase_if(Listeners, [&](const Listener& l) { return l.Id == id; });
}

std::vector<uint64_t> AIEventManager::ListenerIds(const NameString& event) const
{
	std::vector<uint64_t> ids;
	for (const Listener& l : Listeners)
	{
		if (l.Registration.Event == event)
			ids.push_back(l.Id);
	}
	return ids;
}

AIEventManager::Listener* AIEventManager::Find(uint64_t id)
{
	for (Listener& l : Listeners)
	{
		if (l.Id == id)
			return &l;
	}
	return nullptr;
}
