#include "Precomp.h"
#include "TestRunner.h"
#include "VM/Frame.h"
#include "VM/NativeFunc.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include "Packages/Core/Properties/UFloatProperty.h"

namespace
{
	// A native function with one float parameter, registered by name the way the real
	// natives are. The handler records that it was entered before touching its argument,
	// because the marshalling code swallows whatever the handler throws.
	struct NativeNoneFixture
	{
		UClass* Class = nullptr;
		UObject* Instance = nullptr;
		UFunction* Func = nullptr;
		bool HandlerEntered = false;

		explicit NativeNoneFixture(PropertyFlags parmFlags)
		{
			Class = GC::Alloc<UClass>("NoneTestClass", nullptr, ObjectFlags::NoFlags);
			Instance = GC::Alloc<UObject>("NoneTestInstance", Class, ObjectFlags::NoFlags);

			UFloatProperty* parm = GC::Alloc<UFloatProperty>("Value", nullptr, ObjectFlags::NoFlags);
			parm->DataOffset = { 0, 1 };
			parm->PropFlags = parmFlags;

			Func = GC::Alloc<UFunction>("NoneTestNative", Class, ObjectFlags::NoFlags);
			Func->FuncFlags = FunctionFlags::Native;
			Func->NativeStruct = Class;
			Func->Properties.push_back(parm);
			Func->StructSize = sizeof(float);
			Func->StructAlignment = alignof(float);
			Func->NumParms = 1;

			NativeFunctions::NativeByName[{ Func->Name, Class->Name }] = [this](UObject*, ExpressionValue* args)
			{
				HandlerEntered = true;
				args[0].ToFloat();
			};
		}

		~NativeNoneFixture()
		{
			NativeFunctions::NativeByName.erase({ Func->Name, Class->Name });
		}

		ExpressionValue CallWithNone()
		{
			Array<ExpressionValue> args = { ExpressionValue::NothingValue() };
			return Frame::Call(Func, Instance, ArrayView<ExpressionValue>(args.data(), args.size()));
		}
	};
}

TEST(VMAccessedNoneOutParamSkipsNative)
{
	NativeNoneFixture fixture(PropertyFlags::Parm | PropertyFlags::OutParm);

	ExpressionValue result = fixture.CallWithNone();

	CHECK_MSG(!fixture.HandlerEntered, "a native was entered with a None out parameter");
	CHECK(result.GetType() == ExpressionValueType::Nothing);
}

// Pins the narrow rule: only out parameters skip the native. Skipping in-parameters breaks
// latent natives such as MoveTo, whose state code then loops without yielding.
TEST(VMAccessedNoneInParamEntersNative)
{
	NativeNoneFixture fixture(PropertyFlags::Parm);

	fixture.CallWithNone();

	CHECK_MSG(fixture.HandlerEntered, "a native with a None in parameter was skipped");
}

// The native call path writes one ExpressionValue per property into the frame, so the loader
// sizes a native function's frame that way. A fixture that sizes it any other way makes the
// call path assign an ExpressionValue into storage that was never constructed as one.
TEST(VMNativeFixtureSizesFrameLikeTheLoader)
{
	NativeNoneFixture fixture(PropertyFlags::Parm);

	CHECK(fixture.Func->IsNativeFunc);
	CHECK_EQ(fixture.Func->StructSize, fixture.Func->Properties.size() * sizeof(ExpressionValue));
	CHECK_EQ(fixture.Func->StructAlignment, alignof(ExpressionValue));
}
