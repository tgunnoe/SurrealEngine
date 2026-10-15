
#include "Precomp.h"
#include "NativeFuncExtractor.h"
#include "Package/PackageManager.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UFunction.h"
#include "Packages/Core/Properties/UProperty.h"
#include "Packages/Core/Properties/UIntProperty.h"
#include "Packages/Core/Properties/UByteProperty.h"
#include "Packages/Core/Properties/UBoolProperty.h"
#include "Packages/Core/Properties/UObjectProperty.h"
#include "Packages/Core/Properties/UFloatProperty.h"
#include "Packages/Core/Properties/UFixedArrayProperty.h"
#include "Packages/Core/Properties/UArrayProperty.h"
#include "Packages/Core/Properties/UMapProperty.h"
#include "Packages/Core/Properties/UClassProperty.h"
#include "Packages/Core/Properties/UStructProperty.h"
#include "Packages/Core/Properties/UNameProperty.h"
#include "Packages/Core/Properties/UStrProperty.h"
#include "Packages/Core/Properties/UStringProperty.h"
#include "VM/Bytecode.h"
#include "Utils/JsonValue.h"
#include "Utils/File.h"
#include "VM/NativeFunc.h"
#include "Utils/Logger.h"
#include <set>

std::string NativeFuncExtractor::Run(PackageManager* packages)
{
	JsonValue jsonRoot = JsonValue::object();

	//for (std::string pkgname : { "Core", "Engine", "IpDrv", "DeusEx", "Extension" } /* packages->GetPackageNames() */)
	for (NameString pkgname : packages->GetPackageNames())
	{
		if (pkgname == "Editor")
			continue;

		JsonValue jsonPackage = CreatePackageJson(TryLoadClasses(packages, pkgname.ToString(), nullptr));
		if (jsonPackage.properties().size() > 0)
		{
			jsonRoot.add(pkgname.ToString(), jsonPackage);
		}
	}

	return jsonRoot.to_json(true);
}

Array<UClass*> NativeFuncExtractor::TryLoadClasses(PackageManager* packages, const std::string& name, Array<std::string>* skipped)
{
	// Map packages can fail to load and hold no natives; one bad file must not abort the whole walk.
	try
	{
		return packages->GetPackage(name)->GetAllObjects<UClass>();
	}
	catch (const std::exception& e)
	{
		// Exception text carries a native stack trace after the first line.
		std::string what = e.what();
		std::string reason = name + ": " + what.substr(0, what.find('\n'));
		LogMessage("Skipping package " + reason);
		if (skipped)
			skipped->push_back(reason);
		return {};
	}
}

Array<std::string> NativeFuncExtractor::FindMissing(PackageManager* packages, Array<std::string>& skipped)
{
	std::set<std::string> missing;

	for (NameString pkgname : packages->GetPackageNames())
	{
		if (pkgname == "Editor")
			continue;

		for (UClass* cls : TryLoadClasses(packages, pkgname.ToString(), &skipped))
		{
			for (UField* child = cls->Children; child != nullptr; child = child->Next)
			{
				UFunction* func = UClass::TryCast<UFunction>(child);
				if (!func || !AllFlags(func->FuncFlags, FunctionFlags::Native))
					continue;

				// Mirrors the handler lookup in Frame::CallNative (VM/Frame.cpp:306-357), which is
				// the source of truth. Keep this in sync by hand. The one deliberate difference is
				// GetSoundLength at 2054 below, which is reported missing although CallNative would
				// find another native's handler at that index.
				bool found = false;
				if (func->NativeFuncIndex != 0)
				{
					// RegisterHandler drops GetSoundLength at 2054 (index shared with another native), so it
					// never has a handler of its own and CallNative would dispatch to the wrong one.
					bool dropped = func->NativeFuncIndex == 2054 && func->Name == "GetSoundLength";
					size_t index = (size_t)func->NativeFuncIndex;
					found = !dropped && index < NativeFunctions::NativeByIndex.size() && NativeFunctions::NativeByIndex[index];
				}
				else
				{
					UStruct* owner = func->NativeStruct ? func->NativeStruct : cls;
					auto it = NativeFunctions::NativeByName.find({ func->Name, owner->Name });
					found = it != NativeFunctions::NativeByName.end() && it->second;
				}

				if (!found)
					missing.insert(pkgname.ToString() + "." + cls->Name.ToString() + "." + func->Name.ToString() + " index=" + std::to_string(func->NativeFuncIndex));
			}
		}
	}

	return Array<std::string>(missing.begin(), missing.end());
}

JsonValue NativeFuncExtractor::CreatePackageJson(const Array<UClass*>& classes)
{
	JsonValue jsonPackage = JsonValue::object();

	for (UClass* cls : classes)
	{
		JsonValue jsonClass = CreateClassJson(cls);
		if (jsonClass.properties().size() > 0)
		{
			jsonPackage.add(cls->Name.ToString(), jsonClass);
		}
	}

	return jsonPackage;
}

JsonValue NativeFuncExtractor::CreateClassJson(UClass* cls)
{
	JsonValue jsonClass = JsonValue::object();

	for (UField* child = cls->Children; child != nullptr; child = child->Next)
	{
		UFunction* func = UClass::TryCast<UFunction>(child);
		if (func && AllFlags(func->FuncFlags, FunctionFlags::Native))
		{
			jsonClass.add(func->Name.ToString(), CreateFunctionJson(func));
		}
	}

	return jsonClass;
}

JsonValue NativeFuncExtractor::CreateFunctionJson(UFunction* func)
{
	JsonValue jsonFunc = JsonValue::object();
	jsonFunc.add("NativeFuncIndex", JsonValue::number(func->NativeFuncIndex));
	jsonFunc.add("Static", JsonValue::boolean(AllFlags(func->FuncFlags, FunctionFlags::Static)));

	Array<std::string> args;
	for (UField* arg = func->Children; arg != nullptr; arg = arg->Next)
	{
		UProperty* prop = UClass::TryCast<UProperty>(arg);
		if (prop && AllFlags(prop->PropFlags, PropertyFlags::Parm))
		{
			std::string type;
			bool pointer = false;
			bool primitive = false;
			if (UClass::TryCast<UIntProperty>(prop)) { type = "int"; primitive = true; }
			else if (UClass::TryCast<UByteProperty>(prop)) { type = "uint8_t"; primitive = true; }
			else if (UClass::TryCast<UBoolProperty>(prop)) { type = "bool"; primitive = true; }
			else if (UClass::TryCast<UObjectProperty>(prop)) { type = "UObject*"; pointer = true; }
			else if (UClass::TryCast<UFloatProperty>(prop)) { type = "float"; primitive = true; }
			else if (UClass::TryCast<UFixedArrayProperty>(prop)) { type = "UFixedArray*"; pointer = true; }
			else if (UClass::TryCast<UArrayProperty>(prop)) { type = "UArray*"; pointer = true; }
			else if (UClass::TryCast<UMapProperty>(prop)) { type = "UMap*"; pointer = true; }
			else if (UClass::TryCast<UClassProperty>(prop)) { type = "UClass*"; pointer = true; }
			else if (UClass::TryCast<UStructProperty>(prop)) type = UClass::Cast<UStructProperty>(prop)->Struct->Name.ToString();
			else if (UClass::TryCast<UNameProperty>(prop)) type = "NameString";
			else if (UClass::TryCast<UStrProperty>(prop)) type = "std::string";
			else if (UClass::TryCast<UStringProperty>(prop)) type = "std::string";

			if (NameString(type) == "Vector") type = "vec3";

			if (AnyFlags(prop->PropFlags, PropertyFlags::OptionalParm | PropertyFlags::SkipParm))
			{
				if (type == "bool")
					type = "BitfieldBool*";
				else
					type += "*";
			}
			else if (AnyFlags(prop->PropFlags, PropertyFlags::OutParm | PropertyFlags::ReturnParm))
			{
				if (type == "bool")
					type = "BitfieldBool&";
				else
					type += "&";
			}
			else if (!pointer && !primitive)
			{
				type = "const " + type + "&";
			}

			args.push_back(type + " " + arg->Name.ToString());
		}
	}

	JsonValue jsonArgs = JsonValue::array(args);
	jsonFunc.add("Arguments", jsonArgs);
	return jsonFunc;
}
