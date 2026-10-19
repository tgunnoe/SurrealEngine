#include "Precomp.h"
#include "TestRunner.h"
#include "GameFolder.h"
#include "VM/Frame.h"

namespace
{
	std::string Describe(const Array<TraceScriptEntry>& entries)
	{
		std::string out;
		for (const TraceScriptEntry& entry : entries)
			out += "[" + entry.cls + "|" + entry.function + "]";
		return out;
	}

	void CheckRejected(const std::string& text, const std::string& error)
	{
		TraceScriptResult r = ParseTraceScript(text);
		CHECK_MSG(!r.ok(), "accepted malformed text: '" + text + "'");
		CHECK_EQ(r.error, error);
		CHECK_MSG(r.entries.empty(), "rejected text still produced entries: '" + text + "'");
	}

	std::string Format(Array<ExpressionValue> args)
	{
		return Frame::FormatTraceArgs(ArrayView<ExpressionValue>(args.data(), args.size()));
	}

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
}

TEST(HarnessTraceParseOneClass)
{
	TraceScriptResult r = ParseTraceScript("ConPlay");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(Describe(r.entries), std::string("[ConPlay|]"));
}

TEST(HarnessTraceParseSeveralInOrder)
{
	TraceScriptResult r = ParseTraceScript("ConWindow,ConPlayBase,HUDConWindowFirst,ConPlayBase");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(Describe(r.entries), std::string("[ConWindow|][ConPlayBase|][HUDConWindowFirst|][ConPlayBase|]"));
}

TEST(HarnessTraceParseClassAndFunction)
{
	TraceScriptResult r = ParseTraceScript("DeusExPlayer.AddGoal");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(Describe(r.entries), std::string("[DeusExPlayer|AddGoal]"));
}

TEST(HarnessTraceParseMixesClassesAndFunctions)
{
	TraceScriptResult r = ParseTraceScript("ConPlayBase,DeusExPlayer.FindGoal,ConWindow");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(Describe(r.entries), std::string("[ConPlayBase|][DeusExPlayer|FindGoal][ConWindow|]"));
}

TEST(HarnessTraceParseTrimsWhitespace)
{
	TraceScriptResult r = ParseTraceScript("  ConPlay \t, DeusExPlayer.AddNote\n,  Actor  ");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(Describe(r.entries), std::string("[ConPlay|][DeusExPlayer|AddNote][Actor|]"));
}

TEST(HarnessTraceParseTrimsAroundTheDot)
{
	// A class or function name never holds whitespace, so a gap beside the dot cannot be part of one
	TraceScriptResult r = ParseTraceScript("DeusExPlayer . AddGoal,Actor\t.Tick");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(Describe(r.entries), std::string("[DeusExPlayer|AddGoal][Actor|Tick]"));

	CheckRejected("DeusExPlayer. ", "bad entry 'DeusExPlayer.': empty function");
	CheckRejected(" .AddGoal", "bad entry '.AddGoal': empty class");
}

TEST(HarnessTraceParseKeepsSpellingAsWritten)
{
	// Matching is without regard to case, but that happens in the VM through NameString; the parser reports what was typed
	TraceScriptResult r = ParseTraceScript("conplay,PLAYERPAWN.playertick");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(Describe(r.entries), std::string("[conplay|][PLAYERPAWN|playertick]"));
}

TEST(HarnessTraceParseRejectsEmptyValue)
{
	CheckRejected("", "empty value");
}

TEST(HarnessTraceParseRejectsBlankValue)
{
	CheckRejected(" ", "empty value");
	CheckRejected(" \t\r\n ", "empty value");
}

TEST(HarnessTraceParseRejectsTrailingComma)
{
	CheckRejected("ConPlay,", "empty entry in 'ConPlay,'");
}

TEST(HarnessTraceParseRejectsLoneComma)
{
	CheckRejected(",", "empty entry in ','");
}

TEST(HarnessTraceParseRejectsEmptyEntryBetweenCommas)
{
	CheckRejected("A,,B", "empty entry in 'A,,B'");
	CheckRejected(",A", "empty entry in ',A'");
}

TEST(HarnessTraceParseRejectsBlankEntry)
{
	CheckRejected("A, ,B", "empty entry in 'A, ,B'");
}

TEST(HarnessTraceParseRejectsEmptyClass)
{
	CheckRejected(".AddGoal", "bad entry '.AddGoal': empty class");
	CheckRejected("A, .AddGoal", "bad entry '.AddGoal': empty class");
}

TEST(HarnessTraceParseRejectsEmptyFunction)
{
	CheckRejected("DeusExPlayer.", "bad entry 'DeusExPlayer.': empty function");
	CheckRejected("DeusExPlayer. ", "bad entry 'DeusExPlayer.': empty function");
}

TEST(HarnessTraceParseRejectsMoreThanOneDot)
{
	CheckRejected("A.B.C", "bad entry 'A.B.C': more than one dot");
}

TEST(HarnessTraceParseCountsDotsBeforeCheckingParts)
{
	// Each of these is also an empty class or function; the dot count is judged first
	CheckRejected("..", "bad entry '..': more than one dot");
	CheckRejected("A..B", "bad entry 'A..B': more than one dot");
	// A lone dot has no class, and the class is judged before the function
	CheckRejected(".", "bad entry '.': empty class");
}

TEST(HarnessTraceParseReportsTheFirstBadEntry)
{
	CheckRejected("A,B.C.D,E.", "bad entry 'B.C.D': more than one dot");
	CheckRejected("A.,,B", "bad entry 'A.': empty function");
	CheckRejected("A,,B.C.D", "empty entry in 'A,,B.C.D'");
}

TEST(HarnessTraceParseRoundTripsGeneratedLists)
{
	const char* classes[] = { "Actor", "ConPlayBase", "DeusExPlayer", "HUDConWindowFirst", "PlayerPawn", "X" };
	const char* functions[] = { "PlayerTick", "AddGoal", "StartConversation", "Tick", "Z" };
	const char* gaps[] = { "", " ", "\t", "  \n" };

	Lcg rng{ 20260419u };
	for (int round = 0; round < 200; round++)
	{
		Array<TraceScriptEntry> expected;
		std::string text;
		const int count = 1 + (int)(rng.Next() % 5);
		for (int i = 0; i < count; i++)
		{
			TraceScriptEntry entry;
			entry.cls = classes[rng.Next() % 6];
			if (rng.Next() % 2 == 0)
				entry.function = functions[rng.Next() % 5];
			expected.push_back(entry);

			if (i > 0)
				text += ",";
			text += gaps[rng.Next() % 4];
			text += entry.cls;
			if (!entry.function.empty())
				text += "." + entry.function;
			text += gaps[rng.Next() % 4];
		}

		TraceScriptResult r = ParseTraceScript(text);
		CHECK_MSG(r.ok(), "'" + text + "' rejected: " + r.error);
		CHECK_EQ(Describe(r.entries), Describe(expected));
	}
}

TEST(HarnessTraceFormatNoArguments)
{
	CHECK_EQ(Format({}), std::string(""));
}

TEST(HarnessTraceFormatEachType)
{
	Array<ExpressionValue> args = {
		ExpressionValue::StringValue("MeetPaul"),
		ExpressionValue::NameValue(NameString("PaulDenton")),
		ExpressionValue::BoolValue(true),
		ExpressionValue::BoolValue(false),
		ExpressionValue::ByteValue(7),
		ExpressionValue::IntValue(-3),
		ExpressionValue::FloatValue(2.5f),
		ExpressionValue::ObjectValue(nullptr),
		ExpressionValue::NothingValue(),
		ExpressionValue::VectorValue(vec3(1.0f, 2.0f, 3.0f)),
	};
	CHECK_EQ(Format(args), std::string("\"MeetPaul\", 'PaulDenton', true, false, 7, -3, 2.5, None, None, ?"));
}

TEST(HarnessTraceFormatEscapesStrings)
{
	Array<ExpressionValue> args = { ExpressionValue::StringValue("a\"b\\c\nd\re") };
	CHECK_EQ(Format(args), std::string("\"a\\\"b\\\\c\\nd\\re\""));
}

TEST(HarnessTraceFormatCutsStringsAtEightyCharacters)
{
	const std::string eighty(80, 'x');

	Array<ExpressionValue> exact = { ExpressionValue::StringValue(eighty) };
	CHECK_EQ(Format(exact), "\"" + eighty + "\"");

	Array<ExpressionValue> longer = { ExpressionValue::StringValue(eighty + "yyy") };
	CHECK_EQ(Format(longer), "\"" + eighty + "\"");

	// The cut comes before the escaping, so an escape sequence is never split
	Array<ExpressionValue> newlines = { ExpressionValue::StringValue(std::string(81, '\n')) };
	std::string expected = "\"";
	for (int i = 0; i < 80; i++)
		expected += "\\n";
	expected += "\"";
	CHECK_EQ(Format(newlines), expected);
}
