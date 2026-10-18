#include "Precomp.h"
#include "TestRunner.h"
#include "VM/Frame.h"
#include "VM/NativeFunc.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"

// The registry is process-global; every test starts from an empty one.
//
// Owners, names and indices below are made up on purpose: the registry must treat natives
// alike whichever they are, so a test that used real ones could pass on a hard-coded
// exception for them.

namespace
{
	// A native declared the way the loader leaves it: Native flag, owning class and index set.
	UFunction* MakeNative(const char* owner, const char* name, int index)
	{
		UClass* ownerClass = GC::Alloc<UClass>(owner, nullptr, ObjectFlags::NoFlags);
		UFunction* func = GC::Alloc<UFunction>(name, ownerClass, ObjectFlags::NoFlags);
		func->FuncFlags = FunctionFlags::Native;
		func->NativeFuncIndex = index;
		func->NativeStruct = ownerClass;
		return func;
	}

	UObject* MakeInstance()
	{
		UClass* cls = GC::Alloc<UClass>("NativesTestClass", nullptr, ObjectFlags::NoFlags);
		return GC::Alloc<UObject>("NativesTestInstance", cls, ObjectFlags::NoFlags);
	}

	// Registration reports a collision by throwing; a test wants that as a failed check.
	template<typename Registration>
	bool Succeeds(Registration registration)
	{
		try
		{
			registration();
			return true;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	bool RegisterCounting(const char* owner, const char* name, int index, int& calls)
	{
		return Succeeds([&] { NativeFunctions::RegisterHandler(owner, name, index, [&calls](UObject*, ExpressionValue*) { calls++; }); });
	}

	void CallNoArgs(UFunction* func, UObject* instance)
	{
		Frame::Call(func, instance, ArrayView<ExpressionValue>());
	}
}

TEST(NativesSharedIndexRunsEachOwnersHandler)
{
	NativeFunctions::Reset();
	NativeFunctions::ShareIndex(3001, "Right");
	int leftCalls = 0;
	int rightCalls = 0;
	CHECK_MSG(RegisterCounting("Left", "First", 3001, leftCalls), "a shared index must accept its first native");
	CHECK_MSG(RegisterCounting("Right", "Second", 3001, rightCalls), "a shared index must accept its second native");
	UFunction* first = MakeNative("Left", "First", 3001);
	UFunction* second = MakeNative("Right", "Second", 3001);
	UObject* instance = MakeInstance();

	CallNoArgs(first, instance);
	CHECK_EQ(leftCalls, 1);
	CHECK_EQ(rightCalls, 0);

	CallNoArgs(second, instance);
	CHECK_EQ(leftCalls, 1);
	CHECK_EQ(rightCalls, 1);
}

TEST(NativesSharedIndexHandlersAreFoundByName)
{
	NativeFunctions::Reset();
	NativeFunctions::ShareIndex(3001, "Right");
	int calls = 0;
	CHECK_MSG(RegisterCounting("Left", "First", 3001, calls), "a shared index must accept its first native");
	CHECK_MSG(RegisterCounting("Right", "Second", 3001, calls), "a shared index must accept its second native");

	CHECK(NativeFunctions::FindHandler(3001, "First", "Left") != nullptr);
	CHECK(NativeFunctions::FindHandler(3001, "Second", "Right") != nullptr);
	// The index alone names no handler, and a name nobody registered must not borrow one.
	CHECK(NativeFunctions::FindHandler(3001, "Second", "Left") == nullptr);
	CHECK(NativeFunctions::FindHandler(3001, "Third", "Right") == nullptr);
}

TEST(NativesSharedIndexOpcodeResolvesToDeclaredOwner)
{
	UFunction* first = MakeNative("Left", "First", 3001);
	UFunction* second = MakeNative("Right", "Second", 3001);

	// Bytecode carries only the index, so which function it means cannot depend on which
	// class the loader happened to reach first.
	NativeFunctions::Reset();
	NativeFunctions::ShareIndex(3001, "Right");
	CHECK_MSG(Succeeds([&] { NativeFunctions::RegisterNativeFunc(first); }), "declared index rejected its first function");
	CHECK_MSG(Succeeds([&] { NativeFunctions::RegisterNativeFunc(second); }), "declared index rejected its second function");
	CHECK(NativeFunctions::FuncByIndex[3001] == second);

	NativeFunctions::Reset();
	NativeFunctions::ShareIndex(3001, "Right");
	CHECK_MSG(Succeeds([&] { NativeFunctions::RegisterNativeFunc(second); }), "declared index rejected its first function");
	CHECK_MSG(Succeeds([&] { NativeFunctions::RegisterNativeFunc(first); }), "declared index rejected its second function");
	CHECK(NativeFunctions::FuncByIndex[3001] == second);
}

// Two natives on one index are a startup error. The error text must be buildable for
// functions that have no package, like the ones built here.
TEST(NativesUndeclaredIndexClashStillThrows)
{
	NativeFunctions::Reset();
	int calls = 0;
	CHECK(RegisterCounting("Left", "First", 3002, calls));
	CHECK(!RegisterCounting("Right", "Second", 3002, calls));

	UFunction* first = MakeNative("Left", "First", 3002);
	UFunction* second = MakeNative("Right", "Second", 3002);
	NativeFunctions::RegisterNativeFunc(first);
	CHECK_THROWS(NativeFunctions::RegisterNativeFunc(second));
}

// Positive control for the registry at the index the games declare for Greater_StrStr; the
// registration itself is covered by the native coverage goldens. The owner is not Object
// because building that class needs an Engine.
TEST(NativesHandlerAtGreaterStrStrIndexIsReached)
{
	NativeFunctions::Reset();
	int calls = 0;
	CHECK(RegisterCounting("StringOps", "Greater_StrStr", 116, calls));
	UFunction* func = MakeNative("StringOps", "Greater_StrStr", 116);

	CallNoArgs(func, MakeInstance());

	CHECK_EQ(calls, 1);
}
