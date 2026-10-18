
#include "Precomp.h"
#include "NStatLogFile.h"
#include "VM/NativeFunc.h"
#include "Engine.h"

void NStatLogFile::RegisterFunctions()
{
	RegisterVMNativeFunc_0("StatLogFile", "CloseLog", &NStatLogFile::CloseLog, 0);
	RegisterVMNativeFunc_0("StatLogFile", "FileFlush", &NStatLogFile::FileFlush, 0);
	RegisterVMNativeFunc_1("StatLogFile", "FileLog", &NStatLogFile::FileLog, 0);
	RegisterVMNativeFunc_1("StatLogFile", "GetChecksum", &NStatLogFile::GetChecksum, 0);
	RegisterVMNativeFunc_3("StatLogFile", "GetPlayerChecksum", &NStatLogFile::GetPlayerChecksum, 0);
	RegisterVMNativeFunc_0("StatLogFile", "OpenLog", &NStatLogFile::OpenLog, 0);
	RegisterVMNativeFunc_1("StatLogFile", "Watermark", &NStatLogFile::Watermark, 0);
}

void NStatLogFile::CloseLog(UObject* Self)
{
	//engine->LogUnimplemented("StatLogFile.CloseLog");
}

void NStatLogFile::FileFlush(UObject* Self)
{
	//engine->LogUnimplemented("StatLogFile.FileFlush");
}

void NStatLogFile::FileLog(UObject* Self, const std::string& EventString)
{
	//engine->LogUnimplemented("StatLogFile.FileLog(" + EventString + ")");
}

void NStatLogFile::GetChecksum(UObject* Self, std::string& Checksum)
{
	Checksum = "0";
	//engine->LogUnimplemented("StatLogFile.GetChecksum");
}

void NStatLogFile::GetPlayerChecksum(UObject* Self, const std::string& PlayerName, const std::string& Secret, std::string& ReturnValue)
{
	// The engine writes no stat log, so there is no checksum to produce.
	ReturnValue = "";
}

void NStatLogFile::OpenLog(UObject* Self)
{
	//engine->LogUnimplemented("StatLogFile.OpenLog");
}

void NStatLogFile::Watermark(UObject* Self, const std::string& EventString)
{
	//engine->LogUnimplemented("StatLogFile.Watermark");
}
