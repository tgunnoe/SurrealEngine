#include "Precomp.h"
#include "TestRunner.h"
#include "GameFolder.h"

namespace
{
	std::string Describe(const Array<ExecItem>& items)
	{
		std::string out;
		for (const ExecItem& item : items)
			out += "[" + FormatExecItem(item) + "]";
		return out;
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

TEST(HarnessExecParseSingleItem)
{
	ExecScheduleResult r = ParseExecSchedule("5:Fire");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.items.size(), (size_t)1);
	CHECK_EQ(r.items[0].seconds, 5.0);
	CHECK_EQ(r.items[0].command, std::string("Fire"));
}

TEST(HarnessExecParseFractionalSecondsAndMapCommand)
{
	ExecScheduleResult r = ParseExecSchedule("12.5:open DM-Deck16][");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.items.size(), (size_t)1);
	CHECK_EQ(r.items[0].seconds, 12.5);
	CHECK_EQ(r.items[0].command, std::string("open DM-Deck16]["));
}

TEST(HarnessExecParseKeepsEqualTimesInOrder)
{
	ExecScheduleResult same = ParseExecSchedule("5:Fire;5:Fire");
	CHECK_MSG(same.ok(), same.error);
	CHECK_EQ(same.items.size(), (size_t)2);
	CHECK_EQ(same.items[0].command, std::string("Fire"));
	CHECK_EQ(same.items[1].command, std::string("Fire"));

	ExecScheduleResult tied = ParseExecSchedule("5:b;5:a;5:c");
	CHECK_MSG(tied.ok(), tied.error);
	CHECK_EQ(Describe(tied.items), std::string("[5 b][5 a][5 c]"));
}

TEST(HarnessExecParseSortsByTime)
{
	ExecScheduleResult r = ParseExecSchedule("3:b;1:a");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(Describe(r.items), std::string("[1 a][3 b]"));
}

TEST(HarnessExecParseTrimsWhitespace)
{
	ExecScheduleResult r = ParseExecSchedule(" 7 : Fire ");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.items.size(), (size_t)1);
	CHECK_EQ(r.items[0].seconds, 7.0);
	CHECK_EQ(r.items[0].command, std::string("Fire"));
}

TEST(HarnessExecParseSplitsAtFirstColonOnly)
{
	ExecScheduleResult r = ParseExecSchedule("4:say a:b");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.items.size(), (size_t)1);
	CHECK_EQ(r.items[0].seconds, 4.0);
	CHECK_EQ(r.items[0].command, std::string("say a:b"));
}

TEST(HarnessExecParseAcceptsZeroSeconds)
{
	ExecScheduleResult r = ParseExecSchedule("0:Fire");
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.items.size(), (size_t)1);
	CHECK_EQ(r.items[0].seconds, 0.0);
}

TEST(HarnessExecParseRejectsMalformedItems)
{
	const char* bad[] = {
		"",				// the flag was given with no value
		"Fire",			// no colon
		"5",			// no colon
		"5:",			// empty command
		"5:   ",		// command is only whitespace
		":Fire",		// no time
		"abc:Fire",		// not a number
		"-1:Fire",		// negative
		"+5:Fire",		// a sign is not a plain decimal
		"1e3:Fire",		// exponent notation
		"0x10:Fire",	// hexadecimal
		"inf:Fire",		// would parse as a number elsewhere
		"nan:Fire",
		"1.2.3:Fire",	// two decimal points
		".:Fire",		// no digits
		"5:Fire;",		// empty trailing item
		";5:Fire",		// empty leading item
		"5:Fire;;6:Fire",
		"5:Fire;Fire",	// a good item does not excuse a bad one
		"5:Fire;6:",
	};
	for (const char* text : bad)
	{
		ExecScheduleResult r = ParseExecSchedule(text);
		CHECK_MSG(!r.ok(), std::string("accepted malformed text: \"") + text + "\"");
		CHECK_MSG(r.items.empty(), std::string("rejected text still produced items: \"") + text + "\"");
		CHECK_MSG(!r.error.empty(), std::string("rejection has no message: \"") + text + "\"");
	}
}

TEST(HarnessExecParseErrorNamesTheBadItem)
{
	ExecScheduleResult r = ParseExecSchedule("5:Fire;abc:Fire;7:Fire");
	CHECK(!r.ok());
	CHECK_MSG(r.error.find("abc:Fire") != std::string::npos, "message does not quote the bad item: " + r.error);
}

TEST(HarnessExecFormatItem)
{
	CHECK_EQ(FormatExecItem({ 5.0, "Fire" }), std::string("5 Fire"));
	CHECK_EQ(FormatExecItem({ 12.5, "open DM-Deck16][" }), std::string("12.5 open DM-Deck16]["));
	CHECK_EQ(FormatExecItem({ 200.0, "Fire" }), std::string("200 Fire"));
	CHECK_EQ(FormatExecItem({ 0.0, "Fire" }), std::string("0 Fire"));
}

TEST(HarnessExecDueNothingBeforeLogin)
{
	Array<ExecItem> schedule = { { 0.0, "a" }, { 1.0, "b" } };
	CHECK_EQ(DueCommands(schedule, -1.0, 0), (size_t)0);
	CHECK_EQ(DueCommands(schedule, -1000.0, 0), (size_t)0);
	CHECK_EQ(DueCommands({}, -1.0, 0), (size_t)0);
	CHECK_EQ(DueCommands({}, 100.0, 0), (size_t)0);
}

TEST(HarnessExecDueBoundaryIsInclusive)
{
	Array<ExecItem> schedule = { { 0.0, "a" }, { 2.0, "b" } };
	CHECK_EQ(DueCommands(schedule, 0.0, 0), (size_t)1);
	CHECK_EQ(DueCommands(schedule, 1.999, 1), (size_t)1);
	CHECK_EQ(DueCommands(schedule, 2.0, 1), (size_t)2);
}

TEST(HarnessExecDueEachItemExactlyOnce)
{
	Array<ExecItem> schedule = { { 1.0, "a" }, { 2.0, "b" }, { 3.0, "c" } };
	const double samples[] = { -1.0, 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.0, 100.0, 100.0 };

	std::string ran;
	size_t cursor = 0;
	for (double t : samples)
	{
		size_t end = DueCommands(schedule, t, cursor);
		for (; cursor < end; cursor++)
			ran += schedule[cursor].command;
	}
	CHECK_EQ(ran, std::string("abc"));
}

TEST(HarnessExecDueTwoItemsInOneCallInOrder)
{
	Array<ExecItem> schedule = { { 5.0, "first" }, { 5.0, "second" }, { 9.0, "later" } };
	CHECK_EQ(DueCommands(schedule, 5.0, 0), (size_t)2);
	CHECK_EQ(DueCommands(schedule, 6.0, 2), (size_t)2);
	CHECK_EQ(DueCommands(schedule, 9.0, 2), (size_t)3);
	CHECK_EQ(DueCommands(schedule, 9.0, 3), (size_t)3);
}

// Property: for any sorted schedule and any non-decreasing clock, the due ranges tile the
// schedule in order with no gap or overlap, and an item is due exactly when its time has passed.
TEST(HarnessExecDuePartitionsAnyScheduleInOrder)
{
	Lcg rng{ 12345u };
	for (int trial = 0; trial < 200; trial++)
	{
		std::string text;
		size_t count = rng.Next() % 8;
		for (size_t i = 0; i < count; i++)
		{
			if (i > 0)
				text += ";";
			text += std::to_string(rng.Next() % 20) + ":cmd" + std::to_string(i);
		}

		Array<ExecItem> schedule;
		if (count > 0)
		{
			ExecScheduleResult parsed = ParseExecSchedule(text);
			CHECK_MSG(parsed.ok(), parsed.error);
			CHECK_EQ(parsed.items.size(), count);
			schedule = parsed.items;
		}

		double t = -2.0;
		size_t cursor = 0;
		size_t delivered = 0;
		for (int step = 0; step < 40; step++)
		{
			t += (rng.Next() % 100) / 50.0;
			size_t end = DueCommands(schedule, t, cursor);
			CHECK_MSG(end >= cursor && end <= schedule.size(), "range left the schedule for " + text);
			for (size_t i = cursor; i < end; i++)
			{
				CHECK_MSG(t >= 0.0 && schedule[i].seconds <= t, "ran an item before its time for " + text);
				delivered++;
			}
			CHECK_MSG(end == schedule.size() || t < 0.0 || schedule[end].seconds > t, "left a due item behind for " + text);
			cursor = end;
		}
		CHECK_MSG(delivered == cursor, "an item was delivered twice or skipped for " + text);
		CHECK_MSG(cursor == schedule.size(), "an item never became due for " + text);
	}
}
