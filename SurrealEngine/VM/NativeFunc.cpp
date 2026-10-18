
#include "Precomp.h"
#include "NativeFunc.h"
#include "Packages/Core/UFunction.h"

Array<UFunction*> NativeFunctions::FuncByIndex;
Array<NativeFuncHandler> NativeFunctions::NativeByIndex;
std::map<std::pair<NameString, NameString>, NativeFuncHandler> NativeFunctions::NativeByName;
std::map<int, NameString> NativeFunctions::SharedIndices;
Array<LatentRunState> NativeFunctions::LatentActionByIndex;
std::map<LatentRunState, int> NativeFunctions::IndexForLatentAction;

void NativeFunctions::Reset()
{
	FuncByIndex.clear();
	NativeByIndex.clear();
	NativeByName.clear();
	SharedIndices.clear();
	LatentActionByIndex.clear();
	IndexForLatentAction.clear();
}

void NativeFunctions::RegisterHandler(const NameString& className, const NameString& funcName, int nativeIndex, NativeFuncHandler handler)
{
	if (nativeIndex != 0 && SharedIndices.find(nativeIndex) == SharedIndices.end())
	{
		if (NativeByIndex.size() <= (size_t)nativeIndex) NativeByIndex.resize((size_t)nativeIndex + 1);

		if (NativeByIndex[nativeIndex] != nullptr)
			Exception::Throw("Attempted to assign native index "
				+ std::to_string(nativeIndex) + " to " + className.ToString() + "." + funcName.ToString()
				+ ", which was already assigned to a different native function.");

		NativeByIndex[nativeIndex] = handler;
	}
	else
	{
		NativeByName[{ funcName, className }] = handler;
	}
}

static std::string DescribeNative(UFunction* func)
{
	return func->NativeStruct->Name.ToString() + "." + func->Name.ToString();
}

void NativeFunctions::RegisterNativeFunc(UFunction* func)
{
	int nativeIndex = func->NativeFuncIndex;
	
	if (nativeIndex != 0)
	{
		if (FuncByIndex.size() <= (size_t)nativeIndex) FuncByIndex.resize((size_t)nativeIndex + 1);

		auto shared = SharedIndices.find(nativeIndex);
		if (shared != SharedIndices.end())
		{
			if (func->NativeStruct->Name == shared->second)
				FuncByIndex[nativeIndex] = func;
			return;
		}

		if (FuncByIndex[nativeIndex] != nullptr)
			Exception::Throw("Attempted to assign native index "
				+ std::to_string(nativeIndex) + " to " + DescribeNative(func)
				+ ", which was already assigned to " + DescribeNative(FuncByIndex[nativeIndex]) + ".");

		FuncByIndex[nativeIndex] = func;
	}
}

void NativeFunctions::ShareIndex(int nativeIndex, const NameString& opcodeClass)
{
	SharedIndices[nativeIndex] = opcodeClass;
}

const NativeFuncHandler* NativeFunctions::FindHandler(int nativeIndex, const NameString& funcName, const NameString& className)
{
	if (nativeIndex != 0)
	{
		if ((size_t)nativeIndex < NativeByIndex.size() && NativeByIndex[nativeIndex])
			return &NativeByIndex[nativeIndex];

		if (SharedIndices.find(nativeIndex) == SharedIndices.end())
			return nullptr;
	}

	auto it = NativeByName.find({ funcName, className });
	return it != NativeByName.end() && it->second ? &it->second : nullptr;
}

void RegisterLatentAction(int nativeIndex, LatentRunState latentAction)
{
	if (NativeFunctions::LatentActionByIndex.size() <= (size_t)nativeIndex) NativeFunctions::LatentActionByIndex.resize((size_t)nativeIndex + 1);
	NativeFunctions::LatentActionByIndex[nativeIndex] = latentAction;
	NativeFunctions::IndexForLatentAction[latentAction] = nativeIndex;
}
