#include "Precomp.h"
#include "TestRunner.h"
#include "Engine.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include "Packages/Core/Properties/UBoolProperty.h"
#include "Packages/Core/Properties/UByteProperty.h"
#include "Packages/Core/Properties/UFloatProperty.h"
#include "Packages/Core/Properties/UIntProperty.h"
#include "Packages/Core/Properties/UNameProperty.h"
#include "Packages/Core/Properties/UObjectProperty.h"
#include "Packages/Core/Properties/UStrProperty.h"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

// These cover how the words after "call <Function>" become the argument values of a script
// function. Finding the function, resolving actors in a level and making the call need a running
// engine and are checked by running maps.

namespace
{
	// A script function built by hand, without a Package or an Engine: the parameters the exec path
	// walks through Children and Next, in the order they are added.
	struct CallFixture
	{
		UClass* Class = nullptr;
		UFunction* Func = nullptr;
		UObject* Self = nullptr;
		UField* Last = nullptr;

		CallFixture()
		{
			Class = GC::Alloc<UClass>("TestClass", nullptr, ObjectFlags::NoFlags);
			Func = GC::Alloc<UFunction>("TestFunc", Class, ObjectFlags::NoFlags);
			Self = GC::Alloc<UObject>("TestSelf", Class, ObjectFlags::NoFlags);
		}

		template<typename T>
		T* Param(const char* name, PropertyFlags flags = PropertyFlags::Parm)
		{
			T* prop = GC::Alloc<T>(name, nullptr, ObjectFlags::NoFlags);
			prop->PropFlags = flags;
			if (Last)
				Last->Next = prop;
			else
				Func->Children = prop;
			Last = prop;
			return prop;
		}

		template<typename T>
		T* Optional(const char* name)
		{
			return Param<T>(name, PropertyFlags::Parm | PropertyFlags::OptionalParm);
		}
	};

	Array<std::string> Args(std::initializer_list<const char*> words)
	{
		Array<std::string> args;
		for (const char* word : words)
			args.push_back(word);
		return args;
	}

	bool Mentions(const std::string& text, const std::string& part)
	{
		return text.find(part) != std::string::npos;
	}

	// A resolver that must not be reached: the object keywords never need the level.
	CallObjectResolver NoResolver(bool& reached)
	{
		return [&reached](const std::string&, const std::string&, std::string&) -> UObject*
		{
			reached = true;
			return nullptr;
		};
	}

	struct Resolution
	{
		std::string kind;
		std::string value;
	};

	// A resolver that records what it was asked for and answers every request with answer
	CallObjectResolver Recording(std::vector<Resolution>& asked, UObject* answer)
	{
		return [&asked, answer](const std::string& kind, const std::string& value, std::string&) -> UObject*
		{
			asked.push_back({ kind, value });
			return answer;
		};
	}

	// Marshals words against a function whose only parameter is a T named paramName
	template<typename T>
	CallArgsResult MarshalOne(const char* paramName, std::initializer_list<const char*> words)
	{
		CallFixture f;
		f.Param<T>(paramName);
		bool reached = false;
		return MarshalCallArgs(f.Func, Args(words), f.Self, NoResolver(reached));
	}
}

TEST(HarnessCallMarshalsEachScalarType)
{
	CallFixture f;
	f.Param<UByteProperty>("Level");
	f.Param<UIntProperty>("Count");
	f.Param<UBoolProperty>("Flag");
	f.Param<UFloatProperty>("Speed");
	f.Param<UStrProperty>("Text");
	f.Param<UNameProperty>("Label");

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "200", "-7", "TRUE", "1.5", "some Text", "SomeLabel" }), f.Self, NoResolver(reached));
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.values.size(), (size_t)6);

	CHECK(r.values[0].GetType() == ExpressionValueType::ValueByte);
	CHECK_EQ((int)r.values[0].ToByte(), 200);
	CHECK(r.values[1].GetType() == ExpressionValueType::ValueInt);
	CHECK_EQ(r.values[1].ToInt(), -7);
	CHECK(r.values[2].GetType() == ExpressionValueType::ValueBool);
	CHECK(r.values[2].ToBool());
	CHECK(r.values[3].GetType() == ExpressionValueType::ValueFloat);
	CHECK_EQ(r.values[3].ToFloat(), 1.5f);
	CHECK(r.values[4].GetType() == ExpressionValueType::ValueString);
	CHECK_EQ(r.values[4].ToString(), std::string("some Text"));
	CHECK(r.values[5].GetType() == ExpressionValueType::ValueName);
	CHECK_EQ(r.values[5].ToName().ToString(), std::string("SomeLabel"));
}

TEST(HarnessCallBoolTakesDigitsAndWordsWithoutRegardToCase)
{
	const std::pair<const char*, bool> accepted[] = {
		{ "0", false }, { "1", true }, { "true", true }, { "False", false }, { "tRuE", true }, { "FALSE", false } };
	for (const auto& [text, expected] : accepted)
	{
		CallArgsResult r = MarshalOne<UBoolProperty>("Flag", { text });
		CHECK_MSG(r.ok(), std::string("bool argument '") + text + "' refused: " + r.error);
		CHECK_EQ(r.values.size(), (size_t)1);
		CHECK_MSG(r.values[0].ToBool() == expected, std::string("bool argument '") + text + "' has the wrong value");
	}
}

TEST(HarnessCallRefusesABadBoolNamingTheParameter)
{
	for (const char* text : { "yes", "2", "", "truee", "-1" })
	{
		CallArgsResult r = MarshalOne<UBoolProperty>("bForcePlay", { text });
		CHECK_MSG(!r.ok(), std::string("bool argument '") + text + "' was accepted");
		CHECK_MSG(Mentions(r.error, "bForcePlay"), "the refusal does not name the parameter: " + r.error);
	}
}

TEST(HarnessCallRefusesANonNumericIntNamingTheParameter)
{
	for (const char* text : { "abc", "", "3.5", "12abc", "0x10", "99999999999" })
	{
		CallArgsResult r = MarshalOne<UIntProperty>("Credits", { text });
		CHECK_MSG(!r.ok(), std::string("int argument '") + text + "' was accepted");
		CHECK_MSG(Mentions(r.error, "Credits"), "the refusal does not name the parameter: " + r.error);
	}
}

TEST(HarnessCallIntKeepsItsValueThroughDecimalText)
{
	// Small deterministic generator: a failing case must reproduce.
	uint32_t state = 12345u;
	std::vector<int32_t> values = { 0, 1, -1, INT32_MAX, INT32_MIN };
	for (int i = 0; i < 200; i++)
	{
		state = state * 1664525u + 1013904223u;
		values.push_back((int32_t)state);
	}

	for (int32_t value : values)
	{
		std::string text = std::to_string(value);
		CallArgsResult r = MarshalOne<UIntProperty>("Credits", { text.c_str() });
		CHECK_MSG(r.ok(), "int argument '" + text + "' refused: " + r.error);
		CHECK_MSG(r.values.size() == 1 && r.values[0].ToInt() == value, "int argument '" + text + "' changed value");
	}
}

TEST(HarnessCallByteTakesExactlyZeroTo255)
{
	for (int value = -3; value <= 258; value++)
	{
		std::string text = std::to_string(value);
		CallArgsResult r = MarshalOne<UByteProperty>("Level", { text.c_str() });
		const bool inRange = value >= 0 && value <= 255;
		CHECK_MSG(r.ok() == inRange, "byte argument '" + text + (inRange ? "' was refused: " + r.error : std::string("' was accepted")));
		if (inRange)
			CHECK_MSG(r.values.size() == 1 && r.values[0].ToByte() == (uint8_t)value, "byte argument '" + text + "' changed value");
	}
}

TEST(HarnessCallRefusesAnOutOfRangeOrNonNumericByteNamingTheParameter)
{
	for (const char* text : { "256", "-1", "abc", "", "1.5" })
	{
		CallArgsResult r = MarshalOne<UByteProperty>("Mission", { text });
		CHECK_MSG(!r.ok(), std::string("byte argument '") + text + "' was accepted");
		CHECK_MSG(Mentions(r.error, "Mission"), "the refusal does not name the parameter: " + r.error);
	}
}

TEST(HarnessCallRefusesANonNumericFloatNamingTheParameter)
{
	for (const char* text : { "abc", "", "1.5x", "nan", "inf", "1e100" })
	{
		CallArgsResult r = MarshalOne<UFloatProperty>("Radius", { text });
		CHECK_MSG(!r.ok(), std::string("float argument '") + text + "' was accepted");
		CHECK_MSG(Mentions(r.error, "Radius"), "the refusal does not name the parameter: " + r.error);
	}

	CallArgsResult r = MarshalOne<UFloatProperty>("Radius", { "-0.25" });
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.values[0].ToFloat(), -0.25f);
}

TEST(HarnessCallOmittedOptionalParametersBecomeNothing)
{
	CallFixture f;
	f.Param<UIntProperty>("Count");
	f.Optional<UIntProperty>("Extra");
	f.Optional<UBoolProperty>("bFlag");

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "4" }), f.Self, NoResolver(reached));
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.values.size(), (size_t)3);
	CHECK_EQ(r.values[0].ToInt(), 4);
	CHECK(r.values[1].GetType() == ExpressionValueType::Nothing);
	CHECK(r.values[2].GetType() == ExpressionValueType::Nothing);

	// An optional parameter that is given still takes its value
	CallArgsResult given = MarshalCallArgs(f.Func, Args({ "4", "9", "1" }), f.Self, NoResolver(reached));
	CHECK_MSG(given.ok(), given.error);
	CHECK_EQ(given.values[1].ToInt(), 9);
	CHECK(given.values[2].ToBool());
}

TEST(HarnessCallRefusesAnOmittedRequiredParameterNamingIt)
{
	CallFixture f;
	f.Param<UNameProperty>("conName");
	f.Param<UObjectProperty>("conOwner");

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "Prebriefing" }), f.Self, NoResolver(reached));
	CHECK(!r.ok());
	CHECK_MSG(Mentions(r.error, "conOwner"), "the refusal does not name the missing parameter: " + r.error);

	CallArgsResult none = MarshalCallArgs(f.Func, Args({}), f.Self, NoResolver(reached));
	CHECK(!none.ok());
	CHECK_MSG(Mentions(none.error, "conName"), "the refusal does not name the first missing parameter: " + none.error);
}

TEST(HarnessCallRefusesExtraArgumentsNamingTheFunction)
{
	CallFixture f;
	f.Param<UIntProperty>("Count");

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "1", "2" }), f.Self, NoResolver(reached));
	CHECK(!r.ok());
	CHECK_MSG(Mentions(r.error, "TestFunc"), "the refusal does not name the function: " + r.error);

	CallFixture noParms;
	CallArgsResult any = MarshalCallArgs(noParms.Func, Args({ "1" }), noParms.Self, NoResolver(reached));
	CHECK(!any.ok());
	CHECK_MSG(Mentions(any.error, "TestFunc"), "the refusal does not name the function: " + any.error);

	CallArgsResult empty = MarshalCallArgs(noParms.Func, Args({}), noParms.Self, NoResolver(reached));
	CHECK_MSG(empty.ok(), empty.error);
	CHECK(empty.values.empty());
}

TEST(HarnessCallSelfAndNoneNeedNoResolver)
{
	CallFixture f;
	f.Param<UObjectProperty>("First");
	f.Param<UObjectProperty>("Second");
	f.Param<UObjectProperty>("Third");
	f.Param<UObjectProperty>("Fourth");

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "self", "NONE", "Self", "none" }), f.Self, NoResolver(reached));
	CHECK_MSG(r.ok(), r.error);
	CHECK_MSG(!reached, "self and none went to the resolver");
	CHECK_EQ(r.values.size(), (size_t)4);
	for (const ExpressionValue& v : r.values)
		CHECK(v.GetType() == ExpressionValueType::ValueObject);
	CHECK(r.values[0].ToObject() == f.Self);
	CHECK(r.values[1].ToObject() == nullptr);
	CHECK(r.values[2].ToObject() == f.Self);
	CHECK(r.values[3].ToObject() == nullptr);
}

TEST(HarnessCallSelfNeedsAnObjectToStandFor)
{
	CallFixture f;
	f.Param<UObjectProperty>("Target");

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "self" }), nullptr, NoResolver(reached));
	CHECK(!r.ok());
	CHECK_MSG(Mentions(r.error, "Target"), "the refusal does not name the parameter: " + r.error);

	CallArgsResult none = MarshalCallArgs(f.Func, Args({ "none" }), nullptr, NoResolver(reached));
	CHECK_MSG(none.ok(), none.error);
}

TEST(HarnessCallObjectPrefixesReachTheResolverWithKindAndValue)
{
	CallFixture f;
	f.Param<UObjectProperty>("A");
	f.Param<UObjectProperty>("B");
	f.Param<UObjectProperty>("C");
	f.Param<UObjectProperty>("D");

	UObject* found = GC::Alloc<UObject>("Found", f.Class, ObjectFlags::NoFlags);
	std::vector<Resolution> asked;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "name:Trooper0", "CLASS:ScriptedPawn", "Bind:Paul Denton", "bind:A:B" }), f.Self, Recording(asked, found));
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(asked.size(), (size_t)4);
	CHECK_EQ(asked[0].kind, std::string("name"));
	CHECK_EQ(asked[0].value, std::string("Trooper0"));
	CHECK_EQ(asked[1].kind, std::string("class"));
	CHECK_EQ(asked[1].value, std::string("ScriptedPawn"));
	CHECK_EQ(asked[2].kind, std::string("bind"));
	CHECK_EQ(asked[2].value, std::string("Paul Denton"));
	CHECK_EQ(asked[3].kind, std::string("bind"));
	CHECK_EQ(asked[3].value, std::string("A:B"));
	CHECK_EQ(r.values.size(), (size_t)4);
	for (const ExpressionValue& v : r.values)
		CHECK(v.ToObject() == found);
}

TEST(HarnessCallRefusesWithTheResolversMessage)
{
	CallFixture f;
	f.Param<UNameProperty>("conName");
	f.Param<UObjectProperty>("conOwner");

	CallObjectResolver failing = [](const std::string& kind, const std::string& value, std::string& error) -> UObject*
	{
		error = "no actor has the " + kind + " " + value + "; the level has: Alpha, Beta";
		return nullptr;
	};
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "MeetPaul", "bind:Paul" }), f.Self, failing);
	CHECK(!r.ok());
	CHECK_MSG(Mentions(r.error, "no actor has the bind Paul; the level has: Alpha, Beta"), "the refusal lost the resolver's message: " + r.error);
	CHECK_MSG(Mentions(r.error, "conOwner"), "the refusal does not name the parameter: " + r.error);
}

TEST(HarnessCallRefusesAnUnknownPrefixOrFormNamingIt)
{
	CallFixture f;
	f.Param<UObjectProperty>("Target");

	bool reached = false;
	CallArgsResult prefix = MarshalCallArgs(f.Func, Args({ "tag:Door" }), f.Self, NoResolver(reached));
	CHECK(!prefix.ok());
	CHECK_MSG(Mentions(prefix.error, "tag"), "the refusal does not name the prefix: " + prefix.error);
	CHECK_MSG(Mentions(prefix.error, "Target"), "the refusal does not name the parameter: " + prefix.error);

	CallArgsResult bare = MarshalCallArgs(f.Func, Args({ "Door" }), f.Self, NoResolver(reached));
	CHECK(!bare.ok());
	CHECK_MSG(Mentions(bare.error, "Door"), "the refusal does not name the argument: " + bare.error);
	CHECK_MSG(Mentions(bare.error, "Target"), "the refusal does not name the parameter: " + bare.error);
	CHECK_MSG(!reached, "an unknown form went to the resolver");
}

TEST(HarnessCallRefusesAnEmptyObjectValueWithoutAskingTheResolver)
{
	CallFixture f;
	f.Param<UObjectProperty>("Target");

	for (const char* text : { "bind:", "name:", "class:" })
	{
		bool reached = false;
		CallArgsResult r = MarshalCallArgs(f.Func, Args({ text }), f.Self, NoResolver(reached));
		CHECK_MSG(!r.ok(), std::string("'") + text + "' was accepted");
		CHECK_MSG(Mentions(r.error, "Target"), "the refusal does not name the parameter: " + r.error);
		CHECK_MSG(!reached, std::string("'") + text + "' went to the resolver");
	}
}

TEST(HarnessCallRefusesAnOutParameterNamingIt)
{
	CallFixture f;
	f.Param<UIntProperty>("Count");
	f.Param<UIntProperty>("Result", PropertyFlags::Parm | PropertyFlags::OutParm);

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "1", "2" }), f.Self, NoResolver(reached));
	CHECK(!r.ok());
	CHECK_MSG(Mentions(r.error, "Result"), "the refusal does not name the out parameter: " + r.error);

	// Refused whether or not the argument for it was given
	CallArgsResult omitted = MarshalCallArgs(f.Func, Args({ "1" }), f.Self, NoResolver(reached));
	CHECK(!omitted.ok());
	CHECK_MSG(Mentions(omitted.error, "Result"), "the refusal does not name the out parameter: " + omitted.error);
}

TEST(HarnessCallRefusesAParameterTypeItCannotMarshalNamingIt)
{
	CallFixture f;
	f.Param<UIntProperty>("Where")->ValueType = ExpressionValueType::ValueVector;

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "1" }), f.Self, NoResolver(reached));
	CHECK(!r.ok());
	CHECK_MSG(Mentions(r.error, "Where"), "the refusal does not name the parameter: " + r.error);
}

TEST(HarnessCallRefusesAnUnmarshalableTypeEvenWhenItsArgumentIsOptionalAndOmitted)
{
	CallFixture f;
	f.Param<UIntProperty>("Count");
	f.Optional<UIntProperty>("Where")->ValueType = ExpressionValueType::ValueRotator;

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "1" }), f.Self, NoResolver(reached));
	CHECK(!r.ok());
	CHECK_MSG(Mentions(r.error, "Where"), "the refusal does not name the parameter: " + r.error);
}

TEST(HarnessCallTheReturnValueTakesNoArgument)
{
	CallFixture f;
	f.Param<UIntProperty>("Count");
	f.Param<UBoolProperty>("ReturnValue", PropertyFlags::Parm | PropertyFlags::ReturnParm);

	bool reached = false;
	CallArgsResult r = MarshalCallArgs(f.Func, Args({ "3" }), f.Self, NoResolver(reached));
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.values.size(), (size_t)1);
	CHECK_EQ(r.values[0].ToInt(), 3);

	CallArgsResult extra = MarshalCallArgs(f.Func, Args({ "3", "1" }), f.Self, NoResolver(reached));
	CHECK(!extra.ok());
	CHECK_MSG(Mentions(extra.error, "TestFunc"), "the refusal does not name the function: " + extra.error);
}
