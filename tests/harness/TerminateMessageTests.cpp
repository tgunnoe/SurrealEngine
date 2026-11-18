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

	std::string Message(std::exception_ptr current, int unwinding = 0)
	{
		return SmokeLog::TerminateMessage(current, unwinding);
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

TEST(HarnessTerminateMessageDuringUnwinding)
{
	// Thrown while another exception was still unwinding: the runtime may report either throw of
	// a double fault, so the line says that this one is the later and how many were unwinding
	CHECK_EQ(Message(std::make_exception_ptr(std::runtime_error("Could not unmap ScenePass.VertexBuffer")), 1),
		std::string("terminate: Could not unmap ScenePass.VertexBuffer (thrown while 1 other exception was still unwinding; this names the later throw)"));
	CHECK_EQ(Message(std::make_exception_ptr(std::logic_error("bad state")), 2),
		std::string("terminate: bad state (thrown while 2 other exceptions were still unwinding; this names the later throw)"));
	CHECK_EQ(Message(std::make_exception_ptr(42), 1),
		std::string("terminate: unknown exception (thrown while 1 other exception was still unwinding; this names the later throw)"));

	// The note only extends the plain text, whatever the count
	for (int unwinding = 1; unwinding <= 5; unwinding++)
	{
		for (std::exception_ptr current : { std::make_exception_ptr(std::runtime_error("text")), std::make_exception_ptr(OwnWhat()), std::make_exception_ptr(NotAnException{ 7 }) })
		{
			std::string plain = Message(current);
			std::string marked = Message(current, unwinding);
			CHECK(marked.compare(0, plain.size(), plain) == 0);
			CHECK(marked.size() > plain.size());
			CHECK(marked.find(std::to_string(unwinding)) != std::string::npos);
			CHECK(marked.find("unwinding") != std::string::npos);
			CHECK(marked.find("later") != std::string::npos);
		}
	}

	// With nothing in flight there is no throw to mark
	CHECK_EQ(Message(nullptr, 2), std::string("terminate: no exception"));
}
