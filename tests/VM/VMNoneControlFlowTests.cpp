
#include "Precomp.h"
#include "TestRunner.h"
#include "Utils/Logger.h"
#include "VM/Frame.h"
#include "VM/Bytecode.h"
#include "VM/Expression.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"

// A statement that evaluates a None context must log "Accessed None" once, whatever the
// statement then does: JumpIfNot, Return and Switch each replace the result the context
// recorded, so the log has to be written from something the replacement cannot erase.

namespace
{
	// Bytecode allocates its expressions from a private arena, so a test that builds a
	// statement by hand keeps its own alive for the process lifetime. Statements holds raw
	// pointers.
	template<typename T>
	T* NewExpr()
	{
		static std::vector<std::unique_ptr<T>> pool;
		pool.push_back(std::make_unique<T>());
		return pool.back().get();
	}

	// Context(object, Nothing): None when `object` evaluates to None.
	ContextExpression* MakeContext(Expression* object)
	{
		ContextExpression* context = NewExpr<ContextExpression>();
		context->ObjectExpr = object;
		context->ContextExpr = NewExpr<NothingExpression>();
		return context;
	}

	ContextExpression* MakeNoneContext()
	{
		return MakeContext(NewExpr<NoObjectExpression>());
	}

	struct NoneFlowFixture
	{
		UClass* Class = nullptr;
		UObject* Instance = nullptr;
		UFunction* Func = nullptr;
		UFunction* Caller = nullptr;

		// `stream` holds the statements the stream parser should build, which is the only way to
		// get a jump target: Bytecode resolves an offset only for expressions it parsed.
		explicit NoneFlowFixture(const Array<uint8_t>& stream = {})
		{
			Class = GC::Alloc<UClass>("NoneFlowClass", nullptr, ObjectFlags::NoFlags);
			Instance = GC::Alloc<UObject>("NoneFlowInstance", Class, ObjectFlags::NoFlags);
			Func = GC::Alloc<UFunction>("NoneFlowFunc", Class, ObjectFlags::NoFlags);
			Func->Code = std::make_shared<Bytecode>(stream, nullptr);
			Caller = GC::Alloc<UFunction>("NoneFlowCaller", Class, ObjectFlags::NoFlags);
		}

		// The parser reads a Return token through the package version, which a test has no
		// package for, so every fixture ends in a hand-built Return.
		void Append(Expression* statement)
		{
			statement->StatementIndex = static_cast<int>(Func->Code->Statements.size());
			Func->Code->Statements.push_back(statement);
		}

		void AppendReturn(Expression* value = nullptr)
		{
			ReturnExpression* ret = NewExpr<ReturnExpression>();
			ret->Value = value;
			Append(ret);
		}

		std::string ExpectedLine() const
		{
			return "Accessed None in " + Class->Name.ToString() + "." + Func->Name.ToString();
		}

		// Runs the function and returns what it added to the log. The log cannot be cleared,
		// so the run is delimited by its size beforehand.
		Array<LogMessageLine> Run()
		{
			const std::list<LogMessageLine>& log = Logger::Get()->GetLog();
			const size_t mark = log.size();

			// A log line from a running frame takes its source from the calling frame, and from
			// the instance class's package when the frame is alone on the call stack, which a
			// hand-built class does not have. So the call runs beneath a caller frame, as a
			// script call does.
			Frame caller(Instance, Caller, 0, false);
			Frame::Callstack.push_back(&caller);
			struct PopCaller { ~PopCaller() { Frame::Callstack.pop_back(); } } popCaller;

			Frame::Call(Func, Instance, ArrayView<ExpressionValue>());

			Array<LogMessageLine> added;
			auto it = log.begin();
			std::advance(it, mark);
			for (; it != log.end(); ++it)
				added.push_back(*it);
			return added;
		}
	};

	size_t CountNoneLines(const Array<LogMessageLine>& lines)
	{
		size_t count = 0;
		for (const LogMessageLine& line : lines)
		{
			if (line.Text.rfind("Accessed None in", 0) == 0)
				count++;
		}
		return count;
	}

	// JumpIfNot(offset 9, Context(NoObject, Nothing)) followed by a Nothing statement at
	// offset 9. The condition evaluates to Nothing, which is false, so the jump is taken.
	const Array<uint8_t> JumpIfNotNoneThenNothing = {
		0x07, 0x09, 0x00,       // JumpIfNot, offset 9
		0x19,                   // Context
		0x2A,                   //   object: NoObject
		0x00, 0x00, 0x00,       //   null-expression offset, zero-fill size
		0x0B,                   //   context expression: Nothing
		0x0B                    // Nothing, at offset 9
	};

	// JumpIfNot(offset 4, False) followed by a Nothing statement at offset 4.
	const Array<uint8_t> JumpIfNotFalseThenNothing = {
		0x07, 0x04, 0x00,       // JumpIfNot, offset 4
		0x28,                   // False
		0x0B                    // Nothing, at offset 4
	};
}

TEST(VMNoneContextInJumpIfNotConditionIsLogged)
{
	NoneFlowFixture fixture(JumpIfNotNoneThenNothing);
	fixture.AppendReturn();

	Array<LogMessageLine> added = fixture.Run();

	CHECK_EQ(CountNoneLines(added), static_cast<size_t>(1));
}

TEST(VMNoneContextInReturnValueIsLogged)
{
	NoneFlowFixture fixture;
	fixture.AppendReturn(MakeNoneContext());

	Array<LogMessageLine> added = fixture.Run();

	CHECK_EQ(CountNoneLines(added), static_cast<size_t>(1));
}

TEST(VMNoneContextInSwitchConditionIsLogged)
{
	NoneFlowFixture fixture;
	SwitchExpression* switchexpr = NewExpr<SwitchExpression>();
	switchexpr->Condition = MakeNoneContext();
	fixture.Append(switchexpr);
	// ProcessSwitch reads the case statements that follow; a case with no value is the
	// default case and is taken at once.
	fixture.Append(NewExpr<CaseExpression>());
	fixture.AppendReturn();

	Array<LogMessageLine> added = fixture.Run();

	CHECK_EQ(CountNoneLines(added), static_cast<size_t>(1));
	// The callstack names the statement from StatementIndex, which ProcessSwitch moves on to
	// the case statements. A log written after the switch is applied names the case instead.
	CHECK_MSG(added.size() >= 2, "the warning is not followed by a callstack line");
	CHECK_MSG(added[1].Text.ends_with(Frame::GetDisassembly(switchexpr)),
		"the callstack does not name the switch statement: " + added[1].Text);
}

// The statement whose result is AccessedNone on its own already logged before the change;
// this pins that moving the log did not make it log twice.
TEST(VMNoneContextInPlainStatementIsLoggedOnce)
{
	NoneFlowFixture fixture;
	fixture.Append(MakeNoneContext());
	fixture.AppendReturn();

	Array<LogMessageLine> added = fixture.Run();

	CHECK_EQ(CountNoneLines(added), static_cast<size_t>(1));
	// The message is the one the smoke log keys on, followed by the callstack.
	CHECK_EQ(added.front().Text, fixture.ExpectedLine());
	CHECK_MSG(added.size() >= 2 && added[1].Text.rfind("at NoneFlowFunc line ", 0) == 0,
		"the warning is not followed by a callstack line");
}

TEST(VMJumpIfNotWithoutNoneLogsNothing)
{
	NoneFlowFixture fixture(JumpIfNotFalseThenNothing);
	fixture.AppendReturn();

	Array<LogMessageLine> added = fixture.Run();

	CHECK_EQ(CountNoneLines(added), static_cast<size_t>(0));
}

TEST(VMTwoNoneContextsInOneStatementLogOnce)
{
	NoneFlowFixture fixture;
	fixture.AppendReturn(MakeContext(MakeNoneContext()));

	Array<LogMessageLine> added = fixture.Run();

	CHECK_EQ(CountNoneLines(added), static_cast<size_t>(1));
}
