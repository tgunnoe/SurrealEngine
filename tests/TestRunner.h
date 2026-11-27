#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>
#include "Utils/File.h"

#ifdef WIN32
#include <process.h>
#define TEST_GETPID _getpid
#else
#include <unistd.h>
#define TEST_GETPID getpid
#endif

// Thrown by the CHECK macros. Deliberately not derived from std::exception so
// a test body's own catch (const std::exception&) cannot swallow a failure.
struct TestFailure
{
	std::string Message;
};

struct TestCase
{
	const char* Name;
	std::function<void()> Body;
};

std::vector<TestCase>& TestRegistry();

struct TestRegistrar
{
	TestRegistrar(const char* name, std::function<void()> body) { TestRegistry().push_back({ name, std::move(body) }); }
};

// Release builds define NDEBUG, so tests must never rely on assert().
#define TEST(name) \
	static void TestBody_##name(); \
	static TestRegistrar TestRegistrar_##name(#name, &TestBody_##name); \
	static void TestBody_##name()

#define TEST_LOCATION (std::string(__FILE__) + ":" + std::to_string(__LINE__))

#define CHECK(cond) \
	do { if (!(cond)) throw TestFailure{ TEST_LOCATION + ": CHECK failed: " #cond }; } while (0)

#define CHECK_MSG(cond, msg) \
	do { if (!(cond)) throw TestFailure{ TEST_LOCATION + ": " + std::string(msg) }; } while (0)

#define CHECK_EQ(actual, expected) \
	do { \
		auto&& a_ = (actual); \
		auto&& e_ = (expected); \
		if (!(a_ == e_)) \
		{ \
			std::ostringstream s_; \
			s_ << TEST_LOCATION << ": CHECK_EQ failed: " #actual " == " #expected " (actual: " << a_ << ", expected: " << e_ << ")"; \
			throw TestFailure{ s_.str() }; \
		} \
	} while (0)

// Passes only if evaluating expr throws a std::exception.
#define CHECK_THROWS(expr) \
	do { \
		bool threw_ = false; \
		try { (void)(expr); } catch (const std::exception&) { threw_ = true; } \
		if (!threw_) throw TestFailure{ TEST_LOCATION + ": CHECK_THROWS failed: " #expr " did not throw" }; \
	} while (0)

// A file in the system temp directory, deleted on scope exit so a failing check leaves no debris.
// The process id keeps concurrent ctest runs (ctest -j) from colliding on a name.
class TempFile
{
public:
	explicit TempFile(const std::string& name)
	{
		Path = (std::filesystem::temp_directory_path() / ("SurrealTests-" + std::to_string(TEST_GETPID()) + "-" + name)).string();
	}

	~TempFile()
	{
		File::try_delete(Path);
	}

	std::string Path;
};
