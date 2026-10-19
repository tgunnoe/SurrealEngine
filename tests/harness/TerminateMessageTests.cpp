#include "Precomp.h"
#include "TestRunner.h"
#include "Utils/Logger.h"
#include <exception>
#include <stdexcept>
#include <string>
#include <new>

// The terminate handler runs only while the process is dying, so it is not exercised here. These
// cover the text it writes to the smoke log for each kind of exception that can be in flight when
// std::terminate is reached.

namespace
{
	struct OwnWhat : std::exception
	{
		const char* what() const noexcept override { return "text from an overriding what"; }
	};

	// Deliberately not a std::exception: the other kind of thing code can throw.
	struct NotAnException
	{
		int code;
	};

	std::string Message(std::exception_ptr current)
	{
		return SmokeLog::TerminateMessage(current);
	}
}

TEST(HarnessTerminateMessageFromStdException)
{
	CHECK_EQ(Message(std::make_exception_ptr(std::runtime_error("Could not unmap ScenePass.VertexBuffer"))), std::string("terminate: Could not unmap ScenePass.VertexBuffer"));

	// Any derived class reports its own what(), not its base's
	CHECK_EQ(Message(std::make_exception_ptr(std::logic_error("bad state"))), std::string("terminate: bad state"));
	CHECK_EQ(Message(std::make_exception_ptr(OwnWhat())), std::string("terminate: text from an overriding what"));
	CHECK_EQ(Message(std::make_exception_ptr(std::bad_alloc())), std::string("terminate: ") + std::bad_alloc().what());

	// An empty what() still gets the prefix, so the line says what it is
	CHECK_EQ(Message(std::make_exception_ptr(std::runtime_error(""))), std::string("terminate: "));
}

TEST(HarnessTerminateMessageFromOtherException)
{
	CHECK_EQ(Message(std::make_exception_ptr(42)), std::string("terminate: unknown exception"));
	CHECK_EQ(Message(std::make_exception_ptr("text")), std::string("terminate: unknown exception"));
	CHECK_EQ(Message(std::make_exception_ptr(NotAnException{ 7 })), std::string("terminate: unknown exception"));
}

TEST(HarnessTerminateMessageWithoutException)
{
	CHECK_EQ(Message(nullptr), std::string("terminate: no exception"));
	CHECK_EQ(Message(std::exception_ptr()), std::string("terminate: no exception"));
}
