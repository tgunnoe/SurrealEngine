#include "Precomp.h"
#include "TestRunner.h"
#include "Engine.h"
#include <regex>
#include <string>

// These cover how the call command reads its function name: a plain name, or a path of properties
// of the player (each a name, or a name and an index into a static array) ending in the function.
// Finding the properties and calling the function need a running engine and are checked by running
// maps.

namespace
{
	const char* KindName(CallTarget::Kind kind)
	{
		switch (kind)
		{
		case CallTarget::Kind::Plain: return "plain";
		case CallTarget::Kind::Qualified: return "qualified";
		case CallTarget::Kind::Malformed: return "malformed";
		}
		return "unknown";
	}

	// The path as text, an index written as a number: "conPlay.conChoices[0]"
	std::string PathText(const CallTarget& target)
	{
		std::string text;
		for (const CallHop& hop : target.path)
		{
			if (!text.empty())
				text += ".";
			text += hop.property;
			if (hop.index)
				text += "[" + std::to_string(*hop.index) + "]";
		}
		return text;
	}

	// The kind and the function in one text, so that one comparison fails on either
	std::string Describe(const std::string& text)
	{
		const CallTarget target = SplitCallTarget(text);
		return std::string(KindName(target.kind)) + "|" + target.function;
	}

	// The same with the path between them
	std::string DescribePath(const std::string& text)
	{
		const CallTarget target = SplitCallTarget(text);
		return std::string(KindName(target.kind)) + "|" + PathText(target) + "|" + target.function;
	}

	// The malformed cases of a list: each must be refused with a reason
	void CheckAllMalformed(std::initializer_list<const char*> texts)
	{
		for (const char* text : texts)
		{
			const CallTarget target = SplitCallTarget(text);
			CHECK_MSG(target.kind == CallTarget::Kind::Malformed, std::string("'") + text + "' was read as " + KindName(target.kind));
			CHECK_MSG(!target.error.empty(), std::string("'") + text + "' is malformed with no reason given");
		}
	}
}

TEST(CallTargetSplitsAQualifiedNameAtItsDot)
{
	CHECK_EQ(Describe("rootWindow.InvokeLoadScreen"), "qualified|InvokeLoadScreen");
	CHECK_EQ(Describe("a.b"), "qualified|b");

	// The function is kept as written: the lookups compare without regard to case
	CHECK_EQ(Describe("RootWindow.invokeLoadScreen"), "qualified|invokeLoadScreen");
}

TEST(CallTargetIsPlainExactlyWhenThereIsNoDot)
{
	for (const char* text : { "InvokeLoadScreen", "StartConversationByName", "x" })
		CHECK_EQ(Describe(text), std::string("plain|") + text);

	for (const char* text : { "a.b", "a.b.c", ".x", "x.", ".", ".." })
		CHECK_MSG(SplitCallTarget(text).kind != CallTarget::Kind::Plain, std::string("'") + text + "' was taken for a plain name");
}

TEST(CallTargetWalksAPathOfProperties)
{
	// The text before the last '.' is the path, one hop per '.'
	CHECK_EQ(DescribePath("rootWindow.InvokeLoadScreen"), "qualified|rootWindow|InvokeLoadScreen");
	CHECK_EQ(DescribePath("a.b.c"), "qualified|a.b|c");
	CHECK_EQ(DescribePath("rootWindow.nothere.x"), "qualified|rootWindow.nothere|x");

	// The names are kept as written: the lookups compare without regard to case
	CHECK_EQ(DescribePath("RootWindow.Nothere.x"), "qualified|RootWindow.Nothere|x");

	const CallTarget target = SplitCallTarget("conPlay.conWinThird.conChoices[0].PressButton");
	CHECK_EQ(std::string(KindName(target.kind)), "qualified");
	CHECK_EQ(target.function, "PressButton");
	CHECK_EQ(target.path.size(), size_t(3));
	CHECK_EQ(target.path[0].property, "conPlay");
	CHECK(!target.path[0].index);
	CHECK_EQ(target.path[1].property, "conWinThird");
	CHECK(!target.path[1].index);
	CHECK_EQ(target.path[2].property, "conChoices");
	CHECK(target.path[2].index.has_value());
	CHECK_EQ(*target.path[2].index, 0);

	// A path of one hop is the property form: only the index is new
	CHECK_EQ(DescribePath("a[3].b"), "qualified|a[3]|b");
}

TEST(CallTargetReadsAnIndexAsANumber)
{
	CHECK_EQ(DescribePath("a[10].b"), "qualified|a[10]|b");
	CHECK_EQ(DescribePath("a[007].b"), "qualified|a[7]|b");
	CHECK_EQ(DescribePath("a[0].b"), "qualified|a[0]|b");

	// Nine digits is the longest index; the value is read by hand, so no conversion throws
	CHECK_EQ(DescribePath("a[999999999].b"), "qualified|a[999999999]|b");
	CHECK_EQ(DescribePath("a[000000000].b"), "qualified|a[0]|b");

	// Only the hop that carries an index has one
	CHECK_EQ(DescribePath("a[1].b.c[2].d"), "qualified|a[1].b.c[2]|d");
}

TEST(CallTargetRefusesAnEmptyHopOrFunction)
{
	CheckAllMalformed({ ".x", "a..b", "a...b", ".a.b", "[0].b", "a.[0].b", ".", "..", "x.", "a.b.", "a.b[0].", "a[0]." });

	// The control: a splitter that refuses every dotted name would pass the loop above
	CHECK(SplitCallTarget("a.b").error.empty());
	CHECK(SplitCallTarget("a.b.c").error.empty());
}

TEST(CallTargetRefusesABadBracket)
{
	// An unclosed bracket, an empty or non-numeric index, a stray ']' and text after the ']'
	CheckAllMalformed({ "a[.b", "a[0.b", "a[0.5].b", "a[].b", "a[x].b", "a[-1].b", "a[+1].b", "a[ 1].b", "a[1 ].b", "a[0x1].b" });
	CheckAllMalformed({ "a].b", "a]x.b", "a[0]].b", "a]b[0].b", "a[0]x.b", "a[0][1].b", "a[0]b[1].b" });

	// An index of more than nine digits does not fit an int
	CheckAllMalformed({ "a[1000000000].b", "a[9999999999].b", "a[0000000001].b", "a[99999999999999999999999].b" });

	// A bracket on a hop that is not the last is as bad as on the last
	CheckAllMalformed({ "a[.b.c", "a.b].c", "a[x].b.c", "a.b[x].c" });
}

TEST(CallTargetRefusesABracketInAPlainNameOrAFunction)
{
	// An index belongs to a property, which needs a '.' after it for the function
	CheckAllMalformed({ "a[0]", "a[", "a]", "[", "]", "[0]" });
	CheckAllMalformed({ "a.b[0]", "a.b[", "a.b]", "a[0].b[0]", "a.[", "a.]" });

	// The control: a splitter that refuses every bracket would pass the loops above
	CHECK(SplitCallTarget("a[0].b").error.empty());
	CHECK(SplitCallTarget("a.b").error.empty());
}

TEST(CallTargetGivesEachKindOfFaultItsOwnReason)
{
	const char* faults[] = {
		"a..b",			// a hop with no name
		"a.",			// no function
		"a[.b",			// no closing bracket
		"a[x].b",		// the index is not digits
		"a].b",			// a closing bracket alone
		"a[0]x.b",		// text after the closing bracket
		"a[1000000000].b",	// an index too long
		"a[0]",			// a bracket with no path to hold it
		"a.b[0]",		// a bracket in the function
	};
	const size_t count = sizeof(faults) / sizeof(faults[0]);
	for (size_t i = 0; i < count; i++)
	{
		for (size_t j = i + 1; j < count; j++)
			CHECK_MSG(SplitCallTarget(faults[i]).error != SplitCallTarget(faults[j]).error,
				std::string("'") + faults[i] + "' and '" + faults[j] + "' are refused with the same reason");
	}
}

// Every text of a small alphabet is read by the splitter and by a regular expression that states
// the grammar once more, and the two must agree on whether it is a plain name, a path or malformed,
// and on what a path holds. The alphabet has the characters that give the grammar its edges: the
// '.', both brackets, a digit, a sign and a letter.
TEST(CallTargetAgreesWithTheGrammarOnEveryShortText)
{
	// A name or function is any text without a '.' or a bracket; an index is one to nine digits
	const std::regex plain("[^.\\[\\]]+");
	const std::regex qualified("([^.\\[\\]]+(\\[[0-9]{1,9}\\])?\\.)+[^.\\[\\]]+");

	const char alphabet[] = { 'a', '.', '[', ']', '7', '-' };
	const size_t radix = sizeof(alphabet);
	const int maxLength = 6;

	size_t checked = 0;
	size_t qualifiedSeen = 0;
	for (int length = 1; length <= maxLength; length++)
	{
		size_t total = 1;
		for (int k = 0; k < length; k++)
			total *= radix;

		for (size_t n = 0; n < total; n++)
		{
			std::string text;
			for (size_t rest = n, k = 0; k < size_t(length); k++, rest /= radix)
				text += alphabet[rest % radix];

			const CallTarget target = SplitCallTarget(text);
			CallTarget::Kind expected = CallTarget::Kind::Malformed;
			if (std::regex_match(text, plain))
				expected = CallTarget::Kind::Plain;
			else if (std::regex_match(text, qualified))
				expected = CallTarget::Kind::Qualified;

			CHECK_MSG(target.kind == expected, "'" + text + "' was read as " + KindName(target.kind) + ", the grammar says " + KindName(expected));
			CHECK_MSG((target.kind == CallTarget::Kind::Malformed) == !target.error.empty(), "'" + text + "' has a reason exactly when it is malformed");

			if (target.kind == CallTarget::Kind::Plain)
			{
				CHECK_MSG(target.function == text && target.path.empty(), "'" + text + "' is plain, so it is the function and has no path");
			}
			else if (target.kind == CallTarget::Kind::Qualified)
			{
				// The path and the function put back give the text (the digits here carry no zeros)
				CHECK_MSG(!target.path.empty() && !target.function.empty(), "'" + text + "' is a path with no hop or no function");
				CHECK_MSG(PathText(target) + "." + target.function == text, "'" + text + "' was read as " + PathText(target) + " then " + target.function);
				qualifiedSeen++;
			}
			checked++;
		}
	}

	// The control: a loop over nothing, or over names alone, would agree with any splitter
	CHECK(checked > 50000);
	CHECK(qualifiedSeen > 1000);
}
