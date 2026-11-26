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
		int64_t waitNs = 0;
		int64_t nextDeadlineNs = 0;
	};

	inline int64_t PeriodNs(int) { return 0; }
	inline int64_t FirstDeadlineNs(int, int64_t) { return -1; }
	inline Step Pace(int, int64_t, int64_t) { return { -1, -1 }; }
	inline int ClampLimit(int) { return -1; }
	inline int EffectiveLimit(int, std::optional<int>) { return -1; }
	inline std::optional<int> ParseDigits(const std::string&) { return std::nullopt; }
}
