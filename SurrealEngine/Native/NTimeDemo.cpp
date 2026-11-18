#include "Precomp.h"
#include "NTimeDemo.h"
#include "VM/NativeFunc.h"

void NTimeDemo::RegisterFunctions()
{
	RegisterVMNativeFunc_0("TimeDemo", "CloseFile", &NTimeDemo::CloseFile, 0);
	RegisterVMNativeFunc_0("TimeDemo", "OpenFile", &NTimeDemo::OpenFile, 0);
	RegisterVMNativeFunc_1("TimeDemo", "WriteToFile", &NTimeDemo::WriteToFile, 0);
}

// A time demo can save its framerate figures to a file (bSaveToFile). The engine keeps no such
// file, so these do nothing and report nothing.
void NTimeDemo::CloseFile(UObject* Self)
{
}

void NTimeDemo::OpenFile(UObject* Self)
{
}

void NTimeDemo::WriteToFile(UObject* Self, const std::string& Text)
{
}
