#include "Precomp.h"
#include "TestRunner.h"
#include "VM/Frame.h"
#include "VM/Bytecode.h"
#include "VM/Expression.h"
#include "Native/NObject.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include "Packages/Core/Properties/UIntProperty.h"
#include "Packages/Core/Properties/UObjectProperty.h"
#include "Packages/Core/Properties/UStringProperty.h"
#include "Packages/Core/Properties/UStructProperty.h"
#include "Packages/Core/Properties/UArrayProperty.h"

namespace
{
	// A script function built by hand, without a Package or an Engine.
	//
	// The single parameter is optional. It is also flagged as the return value so the
	// function hands back the parameter's frame-local storage, which is the only way for a
	// caller outside the VM to observe what argument marshalling left in it.
	struct OptionalParmFixture
	{
		UClass* Class = nullptr;
		UObject* Instance = nullptr;
		UFunction* Func = nullptr;

		OptionalParmFixture(UProperty* parm, size_t size, size_t alignment)
		{
			Class = GC::Alloc<UClass>("TestClass", nullptr, ObjectFlags::NoFlags);
			Instance = GC::Alloc<UObject>("TestInstance", Class, ObjectFlags::NoFlags);

			parm->DataOffset = { 0, 1 };
			parm->PropFlags = PropertyFlags::Parm | PropertyFlags::OptionalParm | PropertyFlags::ReturnParm;

			Func = GC::Alloc<UFunction>("TestFunc", Class, ObjectFlags::NoFlags);
			Func->Properties.push_back(parm);
			Func->StructSize = size;
			Func->StructAlignment = alignment;
			Func->NumParms = 1;
			Func->Code = std::make_shared<Bytecode>(Array<uint8_t>(), nullptr);

			// Returns with no value expression, which makes the VM yield the return-flagged local.
			// Bytecode allocates its expressions from a private arena, so tests cannot create
			// them through it; keep our own alive for the process lifetime in a function-local
			// static, since Statements holds raw pointers.
			static std::vector<std::unique_ptr<ReturnExpression>> returns;
			returns.push_back(std::make_unique<ReturnExpression>());
			returns.back()->StatementIndex = 0;
			Func->Code->Statements.push_back(returns.back().get());
		}

		ExpressionValue CallWithNoArgs()
		{
			return Frame::Call(Func, Instance, ArrayView<ExpressionValue>());
		}

		ExpressionValue CallWithArgs(Array<ExpressionValue>& args)
		{
			return Frame::Call(Func, Instance, ArrayView<ExpressionValue>(args.data(), args.size()));
		}
	};

	UIntProperty* MakeIntProperty(const char* name, size_t offset)
	{
		UIntProperty* prop = GC::Alloc<UIntProperty>(name, nullptr, ObjectFlags::NoFlags);
		prop->DataOffset = { offset, 1 };
		return prop;
	}

	UStructProperty* MakeIntPairStructProperty()
	{
		UStruct* pair = GC::Alloc<UStruct>("IntPair", nullptr, ObjectFlags::NoFlags);
		pair->Properties.push_back(MakeIntProperty("First", 0));
		pair->Properties.push_back(MakeIntProperty("Second", sizeof(int32_t)));
		pair->StructSize = 2 * sizeof(int32_t);
		pair->StructAlignment = alignof(int32_t);

		UStructProperty* prop = GC::Alloc<UStructProperty>("Opt", nullptr, ObjectFlags::NoFlags);
		prop->Struct = pair;
		return prop;
	}

	// A size-2 int array plus the flag an Array_ native writes its result to. The caller picks the
	// flag's start value so a native that never writes it cannot pass by leaving it alone.
	struct ArrayNativeFixture
	{
		ScriptArray Array;
		uint32_t Flag;
		BitfieldBool Result;

		explicit ArrayNativeFixture(bool startFlag) : Array(MakeIntProperty("Inner", 0)), Flag(startFlag ? 1 : 0), Result{ &Flag, 1 }
		{
			Array.Resize(2);
		}

		ArrayNativeFixture(const ArrayNativeFixture&) = delete;
		ArrayNativeFixture& operator=(const ArrayNativeFixture&) = delete;
	};
}

TEST(CallOmittedOptionalStructParm)
{
	UStructProperty* parm = MakeIntPairStructProperty();
	OptionalParmFixture fixture(parm, 2 * sizeof(int32_t), alignof(int32_t));

	ExpressionValue result = fixture.CallWithNoArgs();

	CHECK(result.GetType() == ExpressionValueType::ValueStruct);
	CHECK_EQ(result.Member(parm->Struct->Properties[0]).ToInt(), 0);
	CHECK_EQ(result.Member(parm->Struct->Properties[1]).ToInt(), 0);
}

TEST(CallOmittedOptionalArrayParm)
{
	UArrayProperty* parm = GC::Alloc<UArrayProperty>("Opt", nullptr, ObjectFlags::NoFlags);
	parm->Inner = MakeIntProperty("Inner", 0);
	OptionalParmFixture fixture(parm, sizeof(ScriptArray), alignof(ScriptArray));

	ExpressionValue result = fixture.CallWithNoArgs();

	CHECK(result.GetType() == ExpressionValueType::ValueArray);
	CHECK_EQ(result.ToArray().GetSize(), (size_t)0);
}

// Positive control: proves the fixture's return channel carries real argument data, so the
// default-value checks above cannot pass vacuously.
TEST(CallPassesStructArg)
{
	UStructProperty* parm = MakeIntPairStructProperty();
	OptionalParmFixture fixture(parm, 2 * sizeof(int32_t), alignof(int32_t));
	int32_t source[2] = { 7, 9 };
	Array<ExpressionValue> args = { ExpressionValue::Variable(source, parm) };

	ExpressionValue result = fixture.CallWithArgs(args);

	CHECK(result.GetType() == ExpressionValueType::ValueStruct);
	CHECK_EQ(result.Member(parm->Struct->Properties[0]).ToInt(), 7);
	CHECK_EQ(result.Member(parm->Struct->Properties[1]).ToInt(), 9);
}

// A Nothing value can reach the argument list itself (a skipped middle argument, a None
// context, a nested call that returned early), so omission by position is not the only case.
TEST(CallExplicitNothingStructParm)
{
	UStructProperty* parm = MakeIntPairStructProperty();
	OptionalParmFixture fixture(parm, 2 * sizeof(int32_t), alignof(int32_t));
	Array<ExpressionValue> args = { ExpressionValue::NothingValue() };

	ExpressionValue result = fixture.CallWithArgs(args);

	CHECK(result.GetType() == ExpressionValueType::ValueStruct);
	CHECK_EQ(result.Member(parm->Struct->Properties[0]).ToInt(), 0);
	CHECK_EQ(result.Member(parm->Struct->Properties[1]).ToInt(), 0);
}

TEST(CallExplicitNothingArrayParm)
{
	UArrayProperty* parm = GC::Alloc<UArrayProperty>("Opt", nullptr, ObjectFlags::NoFlags);
	parm->Inner = MakeIntProperty("Inner", 0);
	OptionalParmFixture fixture(parm, sizeof(ScriptArray), alignof(ScriptArray));
	Array<ExpressionValue> args = { ExpressionValue::NothingValue() };

	ExpressionValue result = fixture.CallWithArgs(args);

	CHECK(result.GetType() == ExpressionValueType::ValueArray);
	CHECK_EQ(result.ToArray().GetSize(), (size_t)0);
}

// A store whose right side is Nothing is what `S = None.Member` produces: the Context handler
// pushes a Nothing value when the context object is None. The original interpreter zeroes
// the result of a None context expression, so the target is reset rather than left alone.

TEST(NullContextStoreResetsStruct)
{
	UStructProperty* prop = MakeIntPairStructProperty();
	prop->DataOffset = { 0, 1 };
	int32_t storage[2] = { 3, 4 };
	ExpressionValue lvalue = ExpressionValue::Variable(storage, prop);

	lvalue.Store(ExpressionValue::NothingValue());

	CHECK_EQ(storage[0], 0);
	CHECK_EQ(storage[1], 0);
}

TEST(NullContextStoreEmptiesDynArray)
{
	UArrayProperty* prop = GC::Alloc<UArrayProperty>("Slot", nullptr, ObjectFlags::NoFlags);
	prop->Inner = MakeIntProperty("Inner", 0);
	prop->DataOffset = { 0, 1 };
	ScriptArray storage(prop->Inner);
	storage.Resize(2);
	ExpressionValue lvalue = ExpressionValue::Variable(&storage, prop);

	lvalue.Store(ExpressionValue::NothingValue());

	CHECK_EQ(storage.GetSize(), (size_t)0);
}

// Positive controls: a scalar target already reaches its zero value through the conversion of
// Nothing, so these pass at every commit and show the Nothing right side is built correctly.
TEST(NullContextStoreZeroesInt)
{
	UIntProperty* prop = MakeIntProperty("Slot", 0);
	int32_t storage = 7;
	ExpressionValue lvalue = ExpressionValue::Variable(&storage, prop);

	lvalue.Store(ExpressionValue::NothingValue());

	CHECK_EQ(storage, 0);
}

TEST(NullContextStoreEmptiesString)
{
	UStringProperty* prop = GC::Alloc<UStringProperty>("Slot", nullptr, ObjectFlags::NoFlags);
	prop->DataOffset = { 0, 1 };
	std::string storage = "x";
	ExpressionValue lvalue = ExpressionValue::Variable(&storage, prop);

	lvalue.Store(ExpressionValue::NothingValue());

	CHECK_EQ(storage, std::string());
}

TEST(NullContextStoreClearsObject)
{
	UObjectProperty* prop = GC::Alloc<UObjectProperty>("Slot", nullptr, ObjectFlags::NoFlags);
	prop->DataOffset = { 0, 1 };
	// Any live object will do; a property is the cheapest one the fixture can make.
	UObject* storage = GC::Alloc<UIntProperty>("Held", nullptr, ObjectFlags::NoFlags);
	ExpressionValue lvalue = ExpressionValue::Variable(&storage, prop);

	lvalue.Store(ExpressionValue::NothingValue());

	CHECK(storage == nullptr);
}

TEST(NullContextStoreZeroesVector)
{
	// A bare struct property is a ValueStruct; only Load marks it as a vector.
	UStructProperty* prop = GC::Alloc<UStructProperty>("Slot", nullptr, ObjectFlags::NoFlags);
	prop->ValueType = ExpressionValueType::ValueVector;
	prop->DataOffset = { 0, 1 };
	vec3 storage(1.0f, 2.0f, 3.0f);
	ExpressionValue lvalue = ExpressionValue::Variable(&storage, prop);

	lvalue.Store(ExpressionValue::NothingValue());

	CHECK(storage == vec3(0.0f, 0.0f, 0.0f));
}

// The Array_ natives take a script offset and count. The two controls are safe on the unchecked
// natives and pass at every commit; the rest need a range check the natives lack. The ones that
// would overflow the heap or run off the end of the buffer are placeholders until the check
// exists; the remove cases run safely today (the new size cannot wrap) and fail by check.
TEST(ArrayNativeInsertAtEndGrows)
{
	ArrayNativeFixture f(false);

	NObject::Array_Insert_U227(f.Array, 2, 1, f.Result);

	CHECK(f.Result.Get());
	CHECK_EQ(f.Array.GetSize(), (size_t)3);
}

TEST(ArrayNativeRemoveLastShrinks)
{
	ArrayNativeFixture f(false);

	NObject::Array_Remove_U227(f.Array, 1, 1, f.Result);

	CHECK(f.Result.Get());
	CHECK_EQ(f.Array.GetSize(), (size_t)1);
}

TEST(ArrayNativeInsertNegativeOffsetReturnsFalse)
{
	CHECK_MSG(false, "Array_Insert with a negative offset cannot be exercised safely: it wraps to a huge size_t and the copy runs off the buffer");
}

TEST(ArrayNativeInsertNegativeCountReturnsFalse)
{
	CHECK_MSG(false, "Array_Insert with a negative count cannot be exercised safely: it wraps to a huge size_t and the construct loop runs off the buffer");
}

TEST(ArrayNativeRemoveRangePastEndReturnsFalse)
{
	ArrayNativeFixture f(true);

	// Offset 1 and count 2 reach one element past the end of a size-2 array.
	NObject::Array_Remove_U227(f.Array, 1, 2, f.Result);

	CHECK_MSG(!f.Result.Get(), "Array_Remove returned true for a range past the end");
	CHECK_EQ(f.Array.GetSize(), (size_t)2);
}

TEST(ArrayNativeScriptArrayInsertPastEndThrows)
{
	CHECK_MSG(false, "ScriptArray::Insert past the end cannot be exercised safely: it reads and writes beyond the buffer");
}

TEST(ArrayNativeScriptArrayRemovePastEndThrows)
{
	ArrayNativeFixture f(false);

	CHECK_THROWS(f.Array.Remove(1, 2));
}

// The tests below pin behavior that is wrong today. They are listed in xfail.txt;
// when one is fixed it reports XPASS and must be removed from that list.

TEST(DynArrayReadOutOfRangeDoesNotResize)
{
	UArrayProperty* prop = GC::Alloc<UArrayProperty>("Arr", nullptr, ObjectFlags::NoFlags);
	prop->Inner = MakeIntProperty("Inner", 0);
	ScriptArray storage(prop->Inner);
	storage.Resize(2);

	ExpressionValue array = ExpressionValue::Variable(&storage, prop);
	array.DynArrayItemAt(5);

	CHECK_EQ(storage.GetSize(), (size_t)2);
}

TEST(StaticArrayOutOfRangeIndexThrows)
{
	UIntProperty* prop = MakeIntProperty("Fixed", 0);
	prop->ArrayDimension = 3;
	int32_t storage[3] = { 10, 20, 30 };

	ExpressionValue array = ExpressionValue::Variable(storage, prop);

	CHECK_THROWS(array.ItemAt(7));
}

TEST(ArrayRemoveOutOfBoundsReturnsFalse)
{
	ScriptArray array(MakeIntProperty("Inner", 0));
	array.Resize(2);
	uint32_t flag = 1;
	BitfieldBool result = { &flag, 1 };

	// Offset equals the size, so no element exists to remove.
	NObject::Array_Remove_U227(array, 2, 1, result);

	CHECK_MSG(!result.Get(), "Array_Remove returned true for an out-of-bounds offset");
}

TEST(ArrayInsertOutOfBoundsReturnsFalse)
{
	// ScriptArray::Insert with an offset past the end writes beyond its new allocation, so this
	// cannot be probed safely until the native checks bounds first. Fail by check, never by crash.
	CHECK_MSG(false, "Array_Insert out-of-bounds cannot be exercised safely: the unchecked call overflows the heap");
}
