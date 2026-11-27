#pragma once

#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include "Packages/Core/Properties/UProperty.h"
#include "VM/ExpressionValue.h"
#include <initializer_list>

// A native function built the way UFunction::Load leaves one. The native call path writes one
// ExpressionValue per property into the frame, so the frame is sized as that and flagged
// IsNativeFunc; sized as a script frame it would receive ExpressionValues in storage that was
// never constructed as such.
//
// The owner is both the function's outer and its NativeStruct. NumParms counts the properties
// flagged Parm, so set each property's flags before building the function.
inline UFunction* MakeNativeFunction(const char* name, UClass* owner, std::initializer_list<UProperty*> properties, int nativeIndex = 0)
{
	UFunction* func = GC::Alloc<UFunction>(name, owner, ObjectFlags::NoFlags);
	func->FuncFlags = FunctionFlags::Native;
	func->NativeStruct = owner;
	func->NativeFuncIndex = nativeIndex;
	for (UProperty* prop : properties)
	{
		func->Properties.push_back(prop);
		if (AllFlags(prop->PropFlags, PropertyFlags::Parm))
			func->NumParms++;
	}
	func->StructAlignment = alignof(ExpressionValue);
	func->StructSize = func->Properties.size() * sizeof(ExpressionValue);
	func->IsNativeFunc = true;
	return func;
}
