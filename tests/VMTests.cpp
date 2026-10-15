#include "Precomp.h"
#include "TestRunner.h"
#include "VM/Frame.h"
#include "VM/Bytecode.h"
#include "VM/Expression.h"
#include "Native/NObject.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include "Packages/Core/Properties/UIntProperty.h"
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
