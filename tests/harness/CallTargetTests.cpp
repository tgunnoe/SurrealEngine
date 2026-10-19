#include "Precomp.h"
#include "TestRunner.h"
#include "Engine.h"
#include <string>

// These cover how the call command reads its function name: a plain name, or one qualified by a
// property of the player. Finding the property and calling the function need a running engine and are
// checked by running maps.

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

	// The kind and both parts in one text, so that one comparison fails on any of them
	std::string Describe(const std::string& text)
	{
		const CallTarget target = SplitCallTarget(text);
		return std::string(KindName(target.kind)) + "|" + target.property + "|" + target.function;
	}
}

TEST(CallTargetSplitsAQualifiedNameAtItsDot)
{
	CHECK_EQ(Describe("rootWindow.InvokeLoadScreen"), "qualified|rootWindow|InvokeLoadScreen");
	CHECK_EQ(Describe("a.b"), "qualified|a|b");

	// The parts are kept as written: the lookups compare without regard to case
	CHECK_EQ(Describe("RootWindow.invokeLoadScreen"), "qualified|RootWindow|invokeLoadScreen");
}

TEST(CallTargetIsPlainExactlyWhenThereIsNoDot)
{
	for (const char* text : { "InvokeLoadScreen", "StartConversationByName", "x" })
		CHECK_EQ(Describe(text), std::string("plain||") + text);

	for (const char* text : { "a.b", "a.b.c", ".x", "x.", ".", ".." })
		CHECK_MSG(SplitCallTarget(text).kind != CallTarget::Kind::Plain, std::string("'") + text + "' was taken for a plain name");
}

TEST(CallTargetRefusesANameWithTooManyOrMissingParts)
{
	for (const char* text : { "a.b.c", ".x", "x.", ".", ".." })
	{
		const CallTarget target = SplitCallTarget(text);
		CHECK_MSG(target.kind == CallTarget::Kind::Malformed, std::string("'") + text + "' was read as " + KindName(target.kind));
		CHECK_MSG(!target.error.empty(), std::string("'") + text + "' is malformed with no reason given");
	}

	// The control: a splitter that refuses every dotted name would pass the loop above
	CHECK(SplitCallTarget("a.b").error.empty());
}
