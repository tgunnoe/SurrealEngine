#pragma once

#include "Utils/JsonValue.h"

class PackageManager;
class UClass;
class UFunction;

class NativeFuncExtractor
{
public:
	static std::string Run(PackageManager* packages);

	// Lines "<Package>.<Class>.<Function> index=<n>" for every script-declared native
	// without a registered handler, sorted and unique.
	// Packages that fail to load are skipped and reported as "<package>: <reason>" in skipped.
	static Array<std::string> FindMissing(PackageManager* packages, Array<std::string>& skipped);

private:
	static Array<UClass*> TryLoadClasses(PackageManager* packages, const std::string& name, Array<std::string>* skipped);
	static JsonValue CreatePackageJson(const Array<UClass*>& classes);
	static JsonValue CreateClassJson(UClass* cls);
	static JsonValue CreateFunctionJson(UFunction* func);
};
