#include "Precomp.h"
#include "TestRunner.h"
#include "Engine.h"
#include <cstdint>
#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <utility>

// These cover the bookkeeping behind the harness's "hold" and "release" console commands: which
// holds are active, when each one ends, and how a command line becomes a hold. Applying a hold to
// the player needs a running engine and is checked by running maps.

namespace
{
	using Hold = InputHoldSchedule::Hold;
	using Kind = InputHoldSchedule::Kind;

	// Small deterministic generator: a failing property case must reproduce.
	struct Lcg
	{
		uint32_t state;
		uint32_t Next()
		{
			state = state * 1664525u + 1013904223u;
			return state >> 8;
		}
	};

	std::string Describe(const Array<Hold>& holds)
	{
		std::string out;
		for (const Hold& h : holds)
			out += std::string("[") + (h.kind == Kind::Button ? "button " : "axis ") + h.name + " " + std::to_string(h.value) + " until " + std::to_string(h.until) + "]";
		return out;
	}

	std::set<std::string> Names(const Array<Hold>& holds)
	{
		std::set<std::string> names;
		for (const Hold& h : holds)
			names.insert(h.name);
		return names;
	}

	Array<std::string> Args(std::initializer_list<const char*> words)
	{
		Array<std::string> args;
		for (const char* word : words)
			args.push_back(word);
		return args;
	}
}

TEST(HarnessHoldStaysActiveUntilItsTime)
{
	InputHoldSchedule schedule;
	schedule.Add(Kind::Button, "bFire", 0.0f, 7.0);
	CHECK_EQ(schedule.Active().size(), (size_t)1);
	CHECK_EQ(schedule.Active()[0].name, std::string("bFire"));
	CHECK_EQ(schedule.Active()[0].until, 7.0);

	CHECK(schedule.TakeExpired(6.9).empty());
	CHECK_EQ(schedule.Active().size(), (size_t)1);

	Array<Hold> expired = schedule.TakeExpired(7.0);
	CHECK_EQ(expired.size(), (size_t)1);
	CHECK_EQ(expired[0].name, std::string("bFire"));
	CHECK(schedule.Active().empty());
}

TEST(HarnessHoldExpiryReturnsOnlyTheDueHolds)
{
	InputHoldSchedule schedule;
	schedule.Add(Kind::Axis, "aForward", 300.0f, 14.0);
	schedule.Add(Kind::Button, "bFire", 0.0f, 7.0);
	schedule.Add(Kind::Axis, "aStrafe", -200.0f, 9.0);

	Array<Hold> expired = schedule.TakeExpired(9.0);
	CHECK(Names(expired) == (std::set<std::string>{ "bFire", "aStrafe" }));
	CHECK(Names(schedule.Active()) == (std::set<std::string>{ "aForward" }));

	// The taken holds carry what the engine needs to end them: the kind and the name
	for (const Hold& h : expired)
		CHECK(h.kind == (h.name == "bFire" ? Kind::Button : Kind::Axis));
}

TEST(HarnessHoldAxisKeepsItsValue)
{
	InputHoldSchedule schedule;
	schedule.Add(Kind::Axis, "aForward", -300.0f, 14.0);
	CHECK_EQ(schedule.Active().size(), (size_t)1);
	CHECK_EQ(schedule.Active()[0].value, -300.0f);
	CHECK(schedule.Active()[0].kind == Kind::Axis);
}

TEST(HarnessHoldSameKindAndNameReplacesTheEarlierHold)
{
	InputHoldSchedule schedule;
	schedule.Add(Kind::Axis, "aForward", 300.0f, 9.0);
	schedule.Add(Kind::Axis, "aForward", -100.0f, 20.0);
	CHECK_EQ(schedule.Active().size(), (size_t)1);
	CHECK_EQ(schedule.Active()[0].value, -100.0f);
	CHECK_EQ(schedule.Active()[0].until, 20.0);

	// A shorter second hold shortens the first; the latest command wins
	schedule.Add(Kind::Axis, "aForward", -100.0f, 12.0);
	CHECK_EQ(schedule.Active().size(), (size_t)1);
	CHECK_EQ(schedule.Active()[0].until, 12.0);
	CHECK(schedule.TakeExpired(11.9).empty());
	CHECK_EQ(schedule.TakeExpired(12.0).size(), (size_t)1);
}

TEST(HarnessHoldButtonAndAxisOfOneNameAreSeparate)
{
	InputHoldSchedule schedule;
	schedule.Add(Kind::Button, "bFoo", 0.0f, 5.0);
	schedule.Add(Kind::Axis, "bFoo", 1.0f, 8.0);
	CHECK_EQ(schedule.Active().size(), (size_t)2);

	Array<Hold> expired = schedule.TakeExpired(5.0);
	CHECK_EQ(expired.size(), (size_t)1);
	CHECK(expired[0].kind == Kind::Button);
	CHECK_EQ(schedule.Active().size(), (size_t)1);
	CHECK(schedule.Active()[0].kind == Kind::Axis);
}

TEST(HarnessHoldTakeAllEndsEveryHoldAtOnce)
{
	InputHoldSchedule schedule;
	schedule.Add(Kind::Button, "bFire", 0.0f, 100.0);
	schedule.Add(Kind::Axis, "aForward", 300.0f, 100.0);
	schedule.Add(Kind::Axis, "aStrafe", 300.0f, 1.0);

	Array<Hold> all = schedule.TakeAll();
	CHECK_EQ(all.size(), (size_t)3);
	CHECK(Names(all) == (std::set<std::string>{ "bFire", "aForward", "aStrafe" }));
	CHECK(schedule.Active().empty());
	CHECK(schedule.TakeAll().empty());
	CHECK(schedule.TakeExpired(1000.0).empty());
}

TEST(HarnessHoldMatchesAModelOverRandomHistories)
{
	// Reference model: the latest hold per (kind, name), ending when the clock reaches its time
	for (uint32_t seed = 1; seed <= 40; seed++)
	{
		Lcg rng{ seed };
		InputHoldSchedule schedule;
		std::map<std::pair<int, std::string>, Hold> model;
		double now = 0.0;
		size_t releasedByTime = 0;

		for (int step = 0; step < 60; step++)
		{
			const uint32_t action = rng.Next() % 8;
			if (action < 4)
			{
				Kind kind = (rng.Next() & 1) ? Kind::Axis : Kind::Button;
				std::string name = "n" + std::to_string(rng.Next() % 4);
				float value = (float)(rng.Next() % 7) - 3.0f;
				double until = now + 0.5 + (rng.Next() % 12) * 0.5;
				schedule.Add(kind, name, value, until);
				model[{ (int)kind, name }] = Hold{ kind, name, value, until };
			}
			else if (action < 7)
			{
				now += (rng.Next() % 6) * 0.5;
				Array<Hold> expired = schedule.TakeExpired(now);
				std::map<std::pair<int, std::string>, Hold> due;
				for (auto it = model.begin(); it != model.end();)
				{
					if (it->second.until <= now)
					{
						due[it->first] = it->second;
						it = model.erase(it);
					}
					else
					{
						++it;
					}
				}
				CHECK_MSG(expired.size() == due.size(), "seed " + std::to_string(seed) + " step " + std::to_string(step) + ": expired " + Describe(expired));
				for (const Hold& h : expired)
				{
					auto found = due.find({ (int)h.kind, h.name });
					CHECK_MSG(found != due.end(), "seed " + std::to_string(seed) + ": unexpected " + Describe({ h }));
					CHECK_EQ(h.value, found->second.value);
					CHECK_EQ(h.until, found->second.until);
				}
				releasedByTime += expired.size();
			}
			else
			{
				Array<Hold> all = schedule.TakeAll();
				CHECK_MSG(all.size() == model.size(), "seed " + std::to_string(seed) + ": TakeAll " + Describe(all));
				model.clear();
			}

			CHECK_MSG(schedule.Active().size() == model.size(), "seed " + std::to_string(seed) + " step " + std::to_string(step) + ": active " + Describe(schedule.Active()));
			for (const Hold& h : schedule.Active())
				CHECK_MSG(h.until > now, "seed " + std::to_string(seed) + ": " + Describe({ h }) + " is overdue at " + std::to_string(now));
		}

		// The generator must have exercised expiry, or the property says nothing
		CHECK_MSG(releasedByTime > 0, "seed " + std::to_string(seed) + " never expired a hold");
	}
}

TEST(HarnessHoldParsesAButtonHold)
{
	HoldParseResult r = ParseHoldCommand(Args({ "hold", "button", "bFire", "2" }), 5.0);
	CHECK_MSG(r.ok(), r.error);
	CHECK(r.hold.kind == Kind::Button);
	CHECK_EQ(r.hold.name, std::string("bFire"));
	CHECK_EQ(r.hold.until, 7.0);
}

TEST(HarnessHoldParsesAnAxisHold)
{
	HoldParseResult r = ParseHoldCommand(Args({ "hold", "axis", "aForward", "300", "4" }), 10.0);
	CHECK_MSG(r.ok(), r.error);
	CHECK(r.hold.kind == Kind::Axis);
	CHECK_EQ(r.hold.name, std::string("aForward"));
	CHECK_EQ(r.hold.value, 300.0f);
	CHECK_EQ(r.hold.until, 14.0);

	HoldParseResult backwards = ParseHoldCommand(Args({ "hold", "axis", "aForward", "-300", "0.5" }), 1.0);
	CHECK_MSG(backwards.ok(), backwards.error);
	CHECK_EQ(backwards.hold.value, -300.0f);
	CHECK_EQ(backwards.hold.until, 1.5);
}

TEST(HarnessHoldParseIgnoresKeywordCase)
{
	HoldParseResult r = ParseHoldCommand(Args({ "Hold", "AXIS", "aForward", "300", "4" }), 0.0);
	CHECK_MSG(r.ok(), r.error);
	CHECK(r.hold.kind == Kind::Axis);
	CHECK_EQ(r.hold.name, std::string("aForward"));
}

TEST(HarnessHoldParseRejectsADurationThatIsNotPositive)
{
	// The same command with a good duration parses, so each rejection below is about the duration
	CHECK(ParseHoldCommand(Args({ "hold", "button", "bFire", "2" }), 5.0).ok());
	CHECK(ParseHoldCommand(Args({ "hold", "axis", "aForward", "300", "4" }), 5.0).ok());

	for (const char* seconds : { "0", "-1", "-0.5", "abc", "2x", "", "nan", "inf", "-inf" })
	{
		HoldParseResult button = ParseHoldCommand(Args({ "hold", "button", "bFire", seconds }), 5.0);
		CHECK_MSG(!button.ok(), std::string("button accepted duration '") + seconds + "'");
		HoldParseResult axis = ParseHoldCommand(Args({ "hold", "axis", "aForward", "300", seconds }), 5.0);
		CHECK_MSG(!axis.ok(), std::string("axis accepted duration '") + seconds + "'");
	}
}

TEST(HarnessHoldParseRejectsAnAxisSpeedThatIsNotANumber)
{
	CHECK(ParseHoldCommand(Args({ "hold", "axis", "aForward", "0", "4" }), 5.0).ok());
	for (const char* speed : { "abc", "300x", "", "nan", "inf" })
	{
		HoldParseResult r = ParseHoldCommand(Args({ "hold", "axis", "aForward", speed, "4" }), 5.0);
		CHECK_MSG(!r.ok(), std::string("accepted speed '") + speed + "'");
	}
}

TEST(HarnessHoldParseRejectsTheWrongShape)
{
	CHECK(ParseHoldCommand(Args({ "hold", "button", "bFire", "2" }), 0.0).ok());

	CHECK(!ParseHoldCommand(Args({ "hold" }), 0.0).ok());
	CHECK(!ParseHoldCommand(Args({ "hold", "button" }), 0.0).ok());
	CHECK(!ParseHoldCommand(Args({ "hold", "button", "bFire" }), 0.0).ok());
	CHECK(!ParseHoldCommand(Args({ "hold", "button", "bFire", "2", "3" }), 0.0).ok());
	CHECK(!ParseHoldCommand(Args({ "hold", "axis", "aForward", "300" }), 0.0).ok());
	CHECK(!ParseHoldCommand(Args({ "hold", "axis", "aForward", "300", "4", "5" }), 0.0).ok());
	CHECK(!ParseHoldCommand(Args({ "hold", "wheel", "bFire", "2" }), 0.0).ok());
}

TEST(HarnessHoldParseErrorsNameTheProblem)
{
	HoldParseResult r = ParseHoldCommand(Args({ "hold", "button", "bFire", "-3" }), 0.0);
	CHECK(!r.ok());
	CHECK_MSG(r.error.find("-3") != std::string::npos, r.error);
}

TEST(HarnessHoldAcceptsAnInputPropertyOfTheRightType)
{
	const PropertyFlags input = PropertyFlags::Input | PropertyFlags::Edit;
	CHECK_EQ(HoldPropertyRefusal(Kind::Button, "bFire", ExpressionValueType::ValueByte, input), std::string());
	CHECK_EQ(HoldPropertyRefusal(Kind::Button, "bExtra0", ExpressionValueType::ValueBool, input), std::string());
	CHECK_EQ(HoldPropertyRefusal(Kind::Axis, "aForward", ExpressionValueType::ValueFloat, input), std::string());

	// The same properties without the flag are refused, so this is not green for a function that accepts everything
	CHECK(!HoldPropertyRefusal(Kind::Button, "bFire", ExpressionValueType::ValueByte, PropertyFlags::Edit).empty());
	CHECK(!HoldPropertyRefusal(Kind::Axis, "aForward", ExpressionValueType::ValueFloat, PropertyFlags::Edit).empty());
}

TEST(HarnessHoldRefusesAPropertyThatIsNotAnInput)
{
	// GroundSpeed and Physics have the right types for an axis and a button but are not inputs
	std::string axis = HoldPropertyRefusal(Kind::Axis, "GroundSpeed", ExpressionValueType::ValueFloat, PropertyFlags::Edit | PropertyFlags::Config);
	CHECK(!axis.empty());
	CHECK_MSG(axis.find("GroundSpeed") != std::string::npos, axis);
	CHECK_MSG(axis.find("not an input") != std::string::npos, axis);

	std::string button = HoldPropertyRefusal(Kind::Button, "Physics", ExpressionValueType::ValueByte, PropertyFlags{});
	CHECK(!button.empty());
	CHECK_MSG(button.find("Physics") != std::string::npos, button);
	CHECK_MSG(button.find("not an input") != std::string::npos, button);

	// An input with the same type is held
	CHECK_EQ(HoldPropertyRefusal(Kind::Axis, "aForward", ExpressionValueType::ValueFloat, PropertyFlags::Input), std::string());
	CHECK_EQ(HoldPropertyRefusal(Kind::Button, "bFire", ExpressionValueType::ValueByte, PropertyFlags::Input), std::string());
}

TEST(HarnessHoldRefusesAnInputOfTheWrongType)
{
	const PropertyFlags input = PropertyFlags::Input;
	CHECK_EQ(HoldPropertyRefusal(Kind::Button, "bFire", ExpressionValueType::ValueByte, input), std::string());
	CHECK_EQ(HoldPropertyRefusal(Kind::Axis, "aForward", ExpressionValueType::ValueFloat, input), std::string());

	// A float is not a button; a byte, bool or int is not an axis
	for (ExpressionValueType type : { ExpressionValueType::ValueFloat, ExpressionValueType::ValueInt, ExpressionValueType::ValueObject })
	{
		std::string r = HoldPropertyRefusal(Kind::Button, "aForward", type, input);
		CHECK_MSG(!r.empty(), "a button accepted type " + std::to_string((int)type));
		CHECK_MSG(r.find("aForward") != std::string::npos, r);
	}
	for (ExpressionValueType type : { ExpressionValueType::ValueByte, ExpressionValueType::ValueBool, ExpressionValueType::ValueInt })
	{
		std::string r = HoldPropertyRefusal(Kind::Axis, "bFire", type, input);
		CHECK_MSG(!r.empty(), "an axis accepted type " + std::to_string((int)type));
		CHECK_MSG(r.find("bFire") != std::string::npos, r);
	}
}
