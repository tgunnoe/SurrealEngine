#pragma once

#include "Packages/Core/UObject.h"

class NStatLogFile
{
public:
	static void RegisterFunctions();

	static void CloseLog(UObject* Self);
	static void FileFlush(UObject* Self);
	static void FileLog(UObject* Self, const std::string& EventString);
	static void GetChecksum(UObject* Self, std::string& Checksum);
	static void GetPlayerChecksum(UObject* Self, const std::string& PlayerName, const std::string& Secret, std::string& ReturnValue);
	static void OpenLog(UObject* Self);
	static void Watermark(UObject* Self, const std::string& EventString);
};
