#include "Precomp.h"
#include "TestRunner.h"
#include "Utils/Logger.h"
#include "VM/Frame.h"
#include "VM/Bytecode.h"
#include "VM/Expression.h"
#include "VM/NativeFunc.h"
#include "VM/NativeFunctionFixture.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include "Packages/Core/Properties/UIntProperty.h"

// Statements whose meaning the VM decides: a call to a function no class has, a state name no class
// declares, a singular function entered again, a None context. Each function body is built by hand
// from expressions, with a native that counts how often a statement ran, so a test can tell whether
// the statement after a failing one was reached.

namespace
{
	// Bytecode keeps raw pointers to its expressions, so a hand-built statement lives for the
	// process lifetime.
	template<typename T>
	T* NewExpr()
	{
		static std::vector<std::unique_ptr<T>> pool;
		pool.push_back(std::make_unique<T>());
		return pool.back().get();
	}

	struct RunOutcome
	{
		Array<LogMessageLine> Added;
		std::string Thrown; // what escaped the call, empty when nothing did
	};

	struct StatementFixture
	{
		UClass* Class = nullptr;
		UObject* Instance = nullptr;
		UObject* Other = nullptr;
		UFunction* Caller = nullptr;
		UFunction* CountFunc = nullptr;
		int Count = 0;
		int Entries = 0;

		StatementFixture()
		{
			Class = GC::Alloc<UClass>("StatementClass", nullptr, ObjectFlags::NoFlags);
			Instance = GC::Alloc<UObject>("StatementInstance", Class, ObjectFlags::NoFlags);
			Other = GC::Alloc<UObject>("StatementOther", Class, ObjectFlags::NoFlags);
			Caller = GC::Alloc<UFunction>("StatementCaller", Class, ObjectFlags::NoFlags);

			CountFunc = MakeNativeFunction("StatementCount", Class, {});
			NativeFunctions::NativeByName[{ CountFunc->Name, Class->Name }] = [this](UObject*, ExpressionValue*) { Count++; };
		}

		~StatementFixture()
		{
			NativeFunctions::NativeByName.erase({ CountFunc->Name, Class->Name });
			Frame::Traces.clear();
			Frame::TraceEntries.clear();
			Frame::Resume();
		}

		StatementFixture(const StatementFixture&) = delete;
		StatementFixture& operator=(const StatementFixture&) = delete;

		UFunction* MakeFunction(const char* name, FunctionFlags flags = {})
		{
			UFunction* func = GC::Alloc<UFunction>(name, Class, ObjectFlags::NoFlags);
			func->Code = std::make_shared<Bytecode>(Array<uint8_t>(), nullptr);
			func->FuncFlags = flags;
			return func;
		}

		// The parser reads a Return token through the package version, which a test has no package
		// for, so statements are appended by hand.
		static void Append(UFunction* func, Expression* statement)
		{
			statement->StatementIndex = static_cast<int>(func->Code->Statements.size());
			func->Code->Statements.push_back(statement);
		}

		static void AppendReturn(UFunction* func)
		{
			Append(func, NewExpr<ReturnExpression>());
		}

		FinalFunctionExpression* CallOf(UFunction* func)
		{
			FinalFunctionExpression* call = NewExpr<FinalFunctionExpression>();
			call->Func = func;
			return call;
		}

		FinalFunctionExpression* Counting() { return CallOf(CountFunc); }

		// A native with no handler: calling it throws, outside the native call's own handler
		UFunction* MakeThrowingNative(const char* name)
		{
			return MakeNativeFunction(name, Class, {});
		}

		static VirtualFunctionExpression* VirtualCall(const char* name)
		{
			VirtualFunctionExpression* call = NewExpr<VirtualFunctionExpression>();
			call->Name = name;
			return call;
		}

		static GlobalFunctionExpression* GlobalCall(const char* name)
		{
			GlobalFunctionExpression* call = NewExpr<GlobalFunctionExpression>();
			call->Name = name;
			return call;
		}

		static ContextExpression* OnObject(UObject* object, Expression* inner)
		{
			ObjectConstExpression* constant = NewExpr<ObjectConstExpression>();
			constant->Object = object;
			ContextExpression* context = NewExpr<ContextExpression>();
			context->ObjectExpr = constant;
			context->ContextExpr = inner;
			return context;
		}

		// A script function that enters itself again would never end if the VM let it, so a call
		// of `func` beyond the allowed entries throws from the trace hook, before its body.
		void LimitEntries(UFunction* func, int allowed)
		{
			Frame::AddTrace(func->Name, [this, allowed](UFunction*, UObject*, const ArrayView<ExpressionValue>&)
			{
				if (++Entries > allowed)
					throw std::runtime_error("entered more than " + std::to_string(allowed) + " times");
			});
		}

		// Runs the function beneath a caller frame, as a script call does, and returns what it
		// added to the log and what escaped it. The log cannot be cleared, so the run is delimited
		// by its size beforehand.
		RunOutcome Run(UFunction* func, UObject* instance = nullptr)
		{
			if (!instance)
				instance = Instance;

			const std::list<LogMessageLine>& log = Logger::Get()->GetLog();
			const size_t mark = log.size();

			RunOutcome outcome;
			{
				// Two frames: a log line written before the called function has its own frame, as
				// a trace line is, takes its source from the class's package when only one is
				// on the call stack, and a hand-built class has none.
				Frame outer(instance, Caller, 0, false);
				Frame inner(instance, Caller, 0, false);
				Frame::Callstack.push_back(&outer);
				Frame::Callstack.push_back(&inner);
				struct PopCallers { ~PopCallers() { Frame::Callstack.pop_back(); Frame::Callstack.pop_back(); } } popCallers;

				try
				{
					Frame::Call(func, instance, ArrayView<ExpressionValue>());
				}
				catch (const std::exception& e)
				{
					outcome.Thrown = e.what();
				}
			}
			// A script exception leaves the VM in its debug break state
			Frame::Resume();

			auto it = log.begin();
			std::advance(it, mark);
			for (; it != log.end(); ++it)
				outcome.Added.push_back(*it);
			return outcome;
		}

		std::string ClassName() const { return Class->Name.ToString(); }
	};

	// The log lines a call added that do not come from tracing
	size_t CountLinesStarting(const Array<LogMessageLine>& lines, const std::string& prefix)
	{
		size_t count = 0;
		for (const LogMessageLine& line : lines)
		{
			if (line.Text.rfind(prefix, 0) == 0)
				count++;
		}
		return count;
	}

	std::string Describe(const RunOutcome& outcome)
	{
		std::string text = "thrown: '" + outcome.Thrown + "', log:";
		for (const LogMessageLine& line : outcome.Added)
			text += " [" + line.Text + "]";
		return text;
	}
}

TEST(MissingVirtualFunctionLogsAScriptErrorAndGoesOn)
{
	StatementFixture f;
	UFunction* func = f.MakeFunction("MissingVirtualBody");
	StatementFixture::Append(func, StatementFixture::VirtualCall("NoSuchVirtualFunction"));
	StatementFixture::Append(func, f.Counting());
	StatementFixture::AppendReturn(func);

	RunOutcome outcome = f.Run(func);

	CHECK_MSG(outcome.Thrown.empty(), "a missing virtual function threw: " + outcome.Thrown);
	CHECK_MSG(outcome.Added.size() == 1, "expected one log line, got " + Describe(outcome));
	CHECK_EQ(outcome.Added[0].Text, "Script error: script function NoSuchVirtualFunction not found in " + f.ClassName());
	CHECK_EQ(f.Count, 1);
}

TEST(MissingGlobalFunctionLogsAScriptErrorAndGoesOn)
{
	StatementFixture f;
	UFunction* func = f.MakeFunction("MissingGlobalBody");
	StatementFixture::Append(func, StatementFixture::GlobalCall("NoSuchGlobalFunction"));
	StatementFixture::Append(func, f.Counting());
	StatementFixture::AppendReturn(func);

	RunOutcome outcome = f.Run(func);

	CHECK_MSG(outcome.Thrown.empty(), "a missing global function threw: " + outcome.Thrown);
	CHECK_MSG(outcome.Added.size() == 1, "expected one log line, got " + Describe(outcome));
	CHECK_EQ(outcome.Added[0].Text, "Script error: script function NoSuchGlobalFunction not found in " + f.ClassName());
	CHECK_EQ(f.Count, 1);
}

TEST(GotoStateUnknownNameLogsTheClassAndTheName)
{
	StatementFixture f;
	const std::list<LogMessageLine>& log = Logger::Get()->GetLog();
	const size_t mark = log.size();

	f.Instance->GotoState("NoSuchState", NameString());

	Array<LogMessageLine> added;
	auto it = log.begin();
	std::advance(it, mark);
	for (; it != log.end(); ++it)
		added.push_back(*it);

	CHECK_EQ(added.size(), static_cast<size_t>(1));
	CHECK_EQ(added[0].Text, "GotoState: no state named NoSuchState in " + f.ClassName());
	CHECK(f.Instance->GetStateName().IsNone());
}

// The names that stay silent: no name, the empty name and a request for the auto state of a class
// that has none.
TEST(GotoStateToNoneEmptyAndAutoLogNothing)
{
	StatementFixture f;
	const std::list<LogMessageLine>& log = Logger::Get()->GetLog();
	const size_t mark = log.size();

	f.Instance->GotoState(NameString(), NameString());
	f.Instance->GotoState("", NameString());
	f.Instance->GotoState("Auto", NameString());

	CHECK_EQ(log.size(), mark);
	CHECK(f.Instance->GetStateName().IsNone());
}

TEST(SingularFunctionEnteredAgainRunsItsBodyOnce)
{
	StatementFixture f;
	UFunction* func = f.MakeFunction("SingularBody", FunctionFlags::Singular);
	StatementFixture::Append(func, f.Counting());
	StatementFixture::Append(func, f.CallOf(func));
	StatementFixture::AppendReturn(func);
	f.LimitEntries(func, 2);

	RunOutcome outcome = f.Run(func);

	CHECK_EQ(f.Count, 1);
	CHECK_MSG(outcome.Thrown.empty(), "the function was entered a third time: " + outcome.Thrown);
}

// The second entry is still a call, so it is still traced: the check that stops its body comes
// after the trace.
TEST(SingularReentrySecondCallIsStillTraced)
{
	StatementFixture f;
	UFunction* func = f.MakeFunction("SingularTracedBody", FunctionFlags::Singular);
	StatementFixture::Append(func, f.Counting());
	StatementFixture::Append(func, f.CallOf(func));
	StatementFixture::AppendReturn(func);
	f.LimitEntries(func, 2);
	Frame::TraceEntries.push_back({ f.Class->Name, func->Name });

	RunOutcome outcome = f.Run(func);

	CHECK_MSG(CountLinesStarting(outcome.Added, "trace: ") == 2, "expected two trace lines for the function: " + Describe(outcome));
}

// A body that throws must not leave the object inside the function, or every later call of it
// would return at once.
TEST(SingularFlagIsClearedWhenTheBodyThrows)
{
	StatementFixture f;
	UFunction* func = f.MakeFunction("SingularThrowingBody", FunctionFlags::Singular);
	StatementFixture::Append(func, f.Counting());
	StatementFixture::Append(func, f.CallOf(f.MakeThrowingNative("StatementNoHandler")));
	StatementFixture::AppendReturn(func);

	RunOutcome first = f.Run(func);

	CHECK_MSG(!first.Thrown.empty(), "the body was meant to throw");
	CHECK(!AnyFlags(f.Instance->Flags, ObjectFlags::InSingularFunc));

	f.Run(func);

	CHECK_EQ(f.Count, 2);
}

TEST(NonSingularFunctionReentersFreely)
{
	StatementFixture f;
	UFunction* func = f.MakeFunction("PlainBody");
	StatementFixture::Append(func, f.Counting());
	StatementFixture::Append(func, f.CallOf(func));
	StatementFixture::AppendReturn(func);
	f.LimitEntries(func, 2);

	f.Run(func);

	CHECK_EQ(f.Count, 2);
}

// The flag belongs to the object: B is not inside the function because A is.
TEST(SingularFlagIsPerObject)
{
	StatementFixture f;
	UFunction* func = f.MakeFunction("SingularPerObjectBody", FunctionFlags::Singular);
	StatementFixture::Append(func, f.Counting());
	StatementFixture::Append(func, StatementFixture::OnObject(f.Other, f.CallOf(func)));
	StatementFixture::AppendReturn(func);
	f.LimitEntries(func, 2);

	f.Run(func);

	CHECK_EQ(f.Count, 2);
}

// A None context makes its expression Nothing, every conversion of which is zero, and the
// statement after it still runs: nothing is skipped.
TEST(ANoneContextStillYieldsZeroAndTheStatementGoesOn)
{
	StatementFixture f;

	UIntProperty* local = GC::Alloc<UIntProperty>("Slot", nullptr, ObjectFlags::NoFlags);
	local->DataOffset = { 0, 1 };

	UIntProperty* parm = GC::Alloc<UIntProperty>("Seen", nullptr, ObjectFlags::NoFlags);
	parm->DataOffset = { 0, 1 };
	parm->PropFlags = PropertyFlags::Parm;
	int seen = -1;
	UFunction* observe = MakeNativeFunction("StatementObserve", f.Class, { parm });
	NativeFunctions::NativeByName[{ observe->Name, f.Class->Name }] = [&seen](UObject*, ExpressionValue* args) { seen = args[0].ToInt(); };

	UFunction* func = f.MakeFunction("NoneContextBody");
	func->Properties.push_back(local);
	func->StructSize = sizeof(int32_t);
	func->StructAlignment = alignof(int32_t);

	auto localRef = [&]()
	{
		LocalVariableExpression* expr = NewExpr<LocalVariableExpression>();
		expr->Variable = local;
		return expr;
	};

	// Slot = 7; Slot = None.Slot; Observe(Slot); Count();
	IntConstExpression* seven = NewExpr<IntConstExpression>();
	seven->Value = 7;
	LetExpression* seed = NewExpr<LetExpression>();
	seed->LeftSide = localRef();
	seed->RightSide = seven;

	InstanceVariableExpression* member = NewExpr<InstanceVariableExpression>();
	member->Variable = local;
	ContextExpression* none = NewExpr<ContextExpression>();
	none->ObjectExpr = NewExpr<NoObjectExpression>();
	none->ContextExpr = member;
	LetExpression* assign = NewExpr<LetExpression>();
	assign->LeftSide = localRef();
	assign->RightSide = none;

	FinalFunctionExpression* observeCall = f.CallOf(observe);
	observeCall->Args.push_back(localRef());

	StatementFixture::Append(func, seed);
	StatementFixture::Append(func, assign);
	StatementFixture::Append(func, observeCall);
	StatementFixture::Append(func, f.Counting());
	StatementFixture::AppendReturn(func);

	RunOutcome outcome = f.Run(func);
	NativeFunctions::NativeByName.erase({ observe->Name, f.Class->Name });

	CHECK_MSG(outcome.Thrown.empty(), "the statement threw: " + outcome.Thrown);
	CHECK_EQ(seen, 0);
	CHECK_EQ(f.Count, 1);
}
