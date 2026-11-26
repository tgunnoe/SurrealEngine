#pragma once

#include <cstdint>
#include <optional>
#include <string>

// Pure frame pacing: every function takes the time as an argument (signed nanoseconds from a monotonic
// clock) and reads no clock, setting or log, so the pacing rules can be tested without an engine.
namespace FramePacer
{
	constexpr int DefaultLimit = 60;
	constexpr int MinLimit = 20;
	constexpr int MaxLimit = 1000;

	struct Step
	{
		int64_t waitNs = 0;			// How long to wait before the next frame starts
		int64_t nextDeadlineNs = 0;	// When the next frame is due to end
	};

	// The length of one frame at a limit; a limit of 0 (off) has none
	inline int64_t PeriodNs(int limit)
	{
		return limit > 0 ? 1'000'000'000 / limit : 0;
	}

	// The first frame is due one period after it started
	inline int64_t FirstDeadlineNs(int limit, int64_t startNs)
	{
		return startNs + PeriodNs(limit);
	}

	// Decides the wait at the end of a frame due at deadlineNs, now that it is nowNs. An on-time frame waits until
	// its deadline and the next one is due a period after it, so the deadlines do not drift with the work done. A
	// frame that ran late (a load, a stall) does not wait and does not leave a debt: the next one is due a period
	// from now, so there is no burst of frames to catch up.
	inline Step Pace(int limit, int64_t deadlineNs, int64_t nowNs)
	{
		if (limit <= 0)
			return { 0, nowNs };

		const int64_t period = PeriodNs(limit);
		if (nowNs >= deadlineNs)
			return { 0, nowNs + period };
		return { deadlineNs - nowNs, deadlineNs + period };
	}

	// What the settings file may hold becomes 0 (off) or 20 to 1000
	inline int ClampLimit(int value)
	{
		if (value <= 0)
			return 0;
		if (value < MinLimit)
			return MinLimit;
		if (value > MaxLimit)
			return MaxLimit;
		return value;
	}

	// A per-run override, already validated, beats the setting, and 0 (off) included
	inline int EffectiveLimit(int setting, std::optional<int> overrideLimit)
	{
		return overrideLimit ? *overrideLimit : ClampLimit(setting);
	}

	// A whole number written in decimal digits only: no sign, space or fraction, and at most nine digits so that
	// it fits an int. A leading zero is read in decimal.
	inline std::optional<int> ParseDigits(const std::string& text)
	{
		if (text.empty() || text.size() > 9)
			return std::nullopt;

		int value = 0;
		for (char c : text)
		{
			if (c < '0' || c > '9')
				return std::nullopt;
			value = value * 10 + (c - '0');
		}
		return value;
	}
}
