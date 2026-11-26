#include "Precomp.h"
#include "TestRunner.h"
#include "GameFolder.h"
#include "LauncherSettings.h"
#include "Utils/FramePacer.h"
#include "Utils/JsonValue.h"

namespace
{
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

	constexpr int64_t Second = 1'000'000'000;

	std::string Describe(const FrameLimitResult& r)
	{
		return r.ok() ? "accepted " + std::to_string(r.limit) : "rejected: " + r.error;
	}

	void CheckAccepted(const std::string& text, int expected)
	{
		FrameLimitResult r = ParseFrameLimit(text);
		CHECK_MSG(r.ok(), "rejected '" + text + "': " + r.error);
		CHECK_MSG(r.limit == expected, "'" + text + "' read as " + std::to_string(r.limit) + ", expected " + std::to_string(expected));
	}

	void CheckRejected(const std::string& text)
	{
		FrameLimitResult r = ParseFrameLimit(text);
		CHECK_MSG(!r.ok(), "accepted '" + text + "': " + Describe(r));
		CHECK_MSG(r.limit == 0, "a rejected text left a limit for '" + text + "'");
		CHECK_MSG(text.empty() || r.error.find(text) != std::string::npos, "the error does not name '" + text + "': " + r.error);
	}
}

TEST(FramePacerOffNeverWaits)
{
	Lcg rng{ 1 };
	for (int i = 0; i < 1000; i++)
	{
		const int64_t deadline = (int64_t)rng.Next() * 1000;
		const int64_t now = deadline + ((int64_t)(rng.Next() % 200'000'000) - 100'000'000);
		CHECK_EQ(FramePacer::Pace(0, deadline, now).waitNs, (int64_t)0);
	}
}

TEST(FramePacerPeriodAt60)
{
	const int64_t period = FramePacer::PeriodNs(60);
	CHECK_MSG(period >= 16'666'667 - 1000 && period <= 16'666'667 + 1000, "period at 60 is " + std::to_string(period));
	CHECK_EQ(FramePacer::PeriodNs(20), (int64_t)50'000'000);
	CHECK_EQ(FramePacer::PeriodNs(1000), (int64_t)1'000'000);
}

TEST(FramePacerLateFrameDoesNotWait)
{
	const int64_t period = FramePacer::PeriodNs(60);
	const int64_t deadline = 5 * Second;

	// Past the deadline by a little, by a lot, and exactly on it: all late, none waits, and the debt is dropped
	for (int64_t late : { (int64_t)1, period / 2, period, 10 * period, (int64_t)0 })
	{
		const int64_t now = deadline + late;
		FramePacer::Step step = FramePacer::Pace(60, deadline, now);
		CHECK_EQ(step.waitNs, (int64_t)0);
		CHECK_EQ(step.nextDeadlineNs, now + period);

		// The frame after a late one, finished early, waits at most one period: no burst of catch-up frames
		FramePacer::Step next = FramePacer::Pace(60, step.nextDeadlineNs, now + period / 4);
		CHECK_MSG(next.waitNs > 0 && next.waitNs <= period, "wait after a late frame is " + std::to_string(next.waitNs));
		CHECK_EQ(next.nextDeadlineNs, now + 2 * period);
	}
}

TEST(FramePacerWaitIsNonIncreasingInLimit)
{
	const int64_t start = 7 * Second;
	for (int64_t work : { (int64_t)0, (int64_t)300'000, (int64_t)3'000'000, (int64_t)20'000'000 })
	{
		int64_t previous = -1;
		for (int limit = 20; limit <= 1000; limit++)
		{
			const int64_t wait = FramePacer::Pace(limit, start + FramePacer::PeriodNs(limit), start + work).waitNs;
			CHECK_MSG(wait >= 0, "negative wait at limit " + std::to_string(limit));
			CHECK_MSG(previous < 0 || wait <= previous, "wait rose from " + std::to_string(previous) + " to " + std::to_string(wait) + " at limit " + std::to_string(limit));
			previous = wait;
		}
	}

	const int64_t at20 = FramePacer::Pace(20, start + FramePacer::PeriodNs(20), start).waitNs;
	const int64_t at1000 = FramePacer::Pace(1000, start + FramePacer::PeriodNs(1000), start).waitNs;
	CHECK_MSG(at20 > at1000, "no work: wait at 20 is " + std::to_string(at20) + ", at 1000 is " + std::to_string(at1000));
}

TEST(FramePacerKeepsNoDriftOver1000Frames)
{
	for (int limit : { 20, 60, 144, 1000 })
	{
		const int64_t period = FramePacer::PeriodNs(limit);
		const int64_t start = 3 * Second;
		Lcg rng{ (uint32_t)limit };

		const int64_t first = FramePacer::FirstDeadlineNs(limit, start);
		CHECK_EQ(first, start + period);

		int64_t deadline = first;
		for (int k = 1; k <= 1000; k++)
		{
			// An on-time frame: it began at the previous deadline and its work ended before this one
			const int64_t work = (int64_t)(rng.Next() % 900) * (period / 1000);
			const int64_t now = deadline - period + work;
			FramePacer::Step step = FramePacer::Pace(limit, deadline, now);
			CHECK_EQ(step.waitNs, deadline - now);
			CHECK_EQ(step.nextDeadlineNs, first + (int64_t)k * period);
			deadline = step.nextDeadlineNs;
		}

		const int64_t exact = start + (int64_t)(1001.0 * (double)Second / limit);
		const int64_t error = deadline > exact ? deadline - exact : exact - deadline;
		CHECK_MSG(error < 1'000'000, "after 1000 frames at " + std::to_string(limit) + " the deadline is " + std::to_string(error) + " ns off");
	}
}

TEST(FramePacerClampsTheLimit)
{
	CHECK_EQ(FramePacer::ClampLimit(-5), 0);
	CHECK_EQ(FramePacer::ClampLimit(0), 0);
	for (int v = 1; v <= 19; v++)
		CHECK_EQ(FramePacer::ClampLimit(v), 20);
	for (int v = 20; v <= 1000; v++)
		CHECK_EQ(FramePacer::ClampLimit(v), v);
	CHECK_EQ(FramePacer::ClampLimit(1001), 1000);
	CHECK_EQ(FramePacer::ClampLimit(2000000000), 1000);

	for (int v = -10; v <= 1100; v++)
		CHECK_EQ(FramePacer::ClampLimit(FramePacer::ClampLimit(v)), FramePacer::ClampLimit(v));
}

TEST(FrameLimitOverrideWinsOverSetting)
{
	CHECK_EQ(FramePacer::EffectiveLimit(30, std::nullopt), 30);
	CHECK_EQ(FramePacer::EffectiveLimit(30, 120), 120);
	CHECK_EQ(FramePacer::EffectiveLimit(30, 0), 0);
	CHECK_EQ(FramePacer::EffectiveLimit(5000, 60), 60);
	CHECK_EQ(FramePacer::EffectiveLimit(5000, std::nullopt), 1000);
	CHECK_EQ(FramePacer::EffectiveLimit(7, std::nullopt), 20);
}

TEST(FrameLimitSettingDefaultsWhenAbsent)
{
	JsonValue absent = JsonValue::object();
	CHECK_EQ(ReadFrameRateLimit(absent), 60);

	JsonValue other = JsonValue::object();
	other["UseVSync"] = JsonValue::boolean(true);
	CHECK_EQ(ReadFrameRateLimit(other), 60);

	JsonValue null = JsonValue::object();
	null["FrameRateLimit"] = JsonValue::null();
	CHECK_EQ(ReadFrameRateLimit(null), 60);

	JsonValue text = JsonValue::object();
	text["FrameRateLimit"] = JsonValue::string("144");
	CHECK_EQ(ReadFrameRateLimit(text), 60);

	JsonValue flag = JsonValue::object();
	flag["FrameRateLimit"] = JsonValue::boolean(true);
	CHECK_EQ(ReadFrameRateLimit(flag), 60);

	JsonValue off = JsonValue::object();
	off["FrameRateLimit"] = JsonValue::number(0);
	CHECK_EQ(ReadFrameRateLimit(off), 0);

	JsonValue set = JsonValue::object();
	set["FrameRateLimit"] = JsonValue::number(144);
	CHECK_EQ(ReadFrameRateLimit(set), 144);
}

TEST(FrameLimitSettingRoundTrips)
{
	for (int limit = 0; limit <= 1000; limit++)
	{
		if (limit > 0 && limit < 20)
			continue;

		JsonValue written = JsonValue::object();
		WriteFrameRateLimit(written, limit);
		const std::string text = written.to_json();
		CHECK_MSG(text.find("\"FrameRateLimit\"") != std::string::npos, "the written text does not name the key: " + text);

		CHECK_EQ(ReadFrameRateLimit(JsonValue::parse(text)), limit);
	}
}

TEST(FrameLimitArgumentParsing)
{
	CheckAccepted("60", 60);
	CheckAccepted("0", 0);
	CheckAccepted("20", 20);
	CheckAccepted("1000", 1000);
	CheckAccepted("060", 60);

	for (const char* bad : { "", "abc", "6x", "6.5", "-1", "+5", " 60", "9999999999", "5000", "1001", "19", "7" })
		CheckRejected(bad);
}

TEST(FrameLimitFieldKeepsPreviousOnNonNumeric)
{
	CHECK_EQ(FrameLimitFromFieldText("abc", 90), 90);
	CHECK_EQ(FrameLimitFromFieldText("", 90), 90);
	CHECK_EQ(FrameLimitFromFieldText("6.5", 90), 90);
	CHECK_EQ(FrameLimitFromFieldText("-1", 90), 90);
	CHECK_EQ(FrameLimitFromFieldText("9999999999", 90), 90);
	CHECK_EQ(FrameLimitFromFieldText("60", 90), 60);
	CHECK_EQ(FrameLimitFromFieldText("060", 90), 60);
	CHECK_EQ(FrameLimitFromFieldText("5", 90), 20);
	CHECK_EQ(FrameLimitFromFieldText("5000", 90), 1000);
	CHECK_EQ(FrameLimitFromFieldText("0", 90), 0);
}
