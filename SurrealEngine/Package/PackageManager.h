#pragma once

#include "GC/GC.h"
#include "Package.h"
#include "IniFile.h"
#include "GameFolder.h"
#include <list>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

// The folder a Deus Ex save slot lives in: "Save" and the slot padded with zeros to four digits, and
// "QuickSave" for the quick-save slot (-1), as in the original game.
std::string DeusExSaveFolderName(int slot);

// Where a save slot lives under the Save folder: Deus Ex keeps one folder per slot, the other games
// one "Save<slot>.<extension>" file.
fs::path SaveSlotPath(bool deusEx, int slot, const fs::path& saveFolder, const std::string& saveExtension);

// Why a game cannot be saved to a slot, or nothing when it can: a slot below -1 is refused in every
// game, and a slot below 0 outside Deus Ex.
std::optional<std::string> SaveSlotRefusal(bool deusEx, int slot);

// Why a save slot cannot be deleted, or nothing when it can: only slots from 1 can, so the quick save
// and the slots below it never are.
std::optional<std::string> SaveSlotDeleteRefusal(int slot);

// Removes the save a slot names under the save folder (the slot's folder on Deus Ex, its
// Save<slot>.<extension> file in the other games), and only when that path, with symbolic links
// resolved, lies inside the save folder. Returns why nothing was removed, or nothing when the save is gone.
std::optional<std::string> RemoveSaveSlot(bool deusEx, int slot, const fs::path& saveFolder, const std::string& saveExtension);

// The lowest slot from 1 whose folder does not exist yet, or 0 when all 9999 are taken.
int NextFreeDeusExSaveIndex(const fs::path& saveFolder);

// The slot folders a Deus Ex save folder holds, as (slot, folder name) in ascending slot order: a
// directory named exactly as DeusExSaveFolderName names a slot of 1 or more. The quick save, slot 0,
// files and every other name are left out, and a folder that is not there lists nothing.
std::vector<std::pair<int, std::string>> ListDeusExSaveSlots(const fs::path& saveFolder);

// The level package of a Deus Ex save folder: its newest file with the save extension other than
// SaveInfo. Nothing when the folder holds none.
std::optional<fs::path> NewestDeusExSaveLevel(const fs::path& slotFolder, const std::string& saveExtension);

class PackageStream;
class UObject;
class UClass;

struct IntObject
{
	NameString Name;
	NameString Class;
	NameString MetaClass;
	std::string Description;
};

struct NativeClass
{
	NameString Package;
	NameString Name;
	NameString Base;
	std::function<UObject* (const NameString& name, UClass* cls, ObjectFlags flags)> CreateFunc;
};

class PackageManager
{
public:
	PackageManager(const GameLaunchInfo& launchInfo);

	bool IsUnreal1() const { return launchInfo.IsUnreal1(); }
	bool IsUnreal1_226() const { return launchInfo.IsUnreal1_226(); }
	bool IsUnreal1_227() const { return launchInfo.IsUnreal1_227(); }
	bool IsUnreal1_227k() const { return launchInfo.IsUnreal1_227k(); }
	bool IsUnrealTournament() const { return launchInfo.IsUnrealTournament(); }
	bool IsUnrealTournament_469() const { return launchInfo.IsUnrealTournament_469(); }
	bool IsDeusEx() const { return launchInfo.IsDeusEx(); }
	bool IsCliveBarkersUndying() const { return launchInfo.IsCliveBarkersUndying(); }
	bool IsKlingonHonorGuard() const  { return launchInfo.IsKlingonHonorGuard(); }
	bool IsRune() const { return launchInfo.IsRune(); }

	fs::path GetRootFolderPath() const { return gameRootFolderPath; }
	fs::path GetSystemFolderPath() const { return gameSystemFolderPath; }
	fs::path GetConfigFolderPath() const { return gameConfigFolderPath; }
	fs::path GetDataFolderPath() const { return gameDataFolderPath; }
	fs::path GetSaveFolderPath() const { return gameSaveFolderPath; }
	fs::path GetCacheFolderPath() const { return gameCacheFolderPath; }

	Package* GetPackage(const NameString& name);
	Array<NameString> GetPackageNames() const;
	Package* GetSaveInfoPackage(const NameString& saveFolderName);
	void RemoveSaveInfoPackage(const NameString& saveFolderName);
	void RegisterSaveInfo(const std::string& saveFolderName);
	std::map<NameString, Package*> GetSaveInfoPackages() const { return saveInfos; };

	Package* LoadMap(const std::string& path);
	void UnloadPackage(Package* package);

	// Drops the cached streams of a package: the cache is keyed by the package's address alone, so
	// one left behind would be handed to whatever package is later allocated there
	void ForgetStreams(Package* package);

	void CloseStreams();

	Package* LoadSaveSlot(int slotNum);

	std::shared_ptr<PackageStream> GetStream(Package* package);

	UClass* FindClass(const NameString& name);

	std::string GetMapExtension() const { return mapExtension; }
	std::string GetSaveExtension() const { return saveExtension; }

	std::unique_ptr<IniFile> GetIniFile(NameString iniName);
	std::unique_ptr<IniFile> GetUserIniFile();
	std::unique_ptr<IniFile> GetSystemIniFile();
	Array<NameString> GetIniKeysFromSection(NameString iniName, const NameString& sectionName);
	std::string GetIniValue(NameString iniName, const NameString& sectionName, const NameString& keyName, std::string default_value = "", const int index = 0);
	Array<std::string> GetIniValues(NameString iniName, const NameString& sectionName, const NameString& keyName, Array<std::string> default_values = {});
	std::string GetDefaultIniValue(const NameString& sectionName, const NameString& keyName, std::string default_value = "", const int index = 0);
	Array<std::string> GetDefaultIniValues(const NameString& sectionName, const NameString& keyName, Array<std::string> default_values = {});
	std::string GetDefUserIniValue(const NameString& sectionName, const NameString& keyName, std::string default_value = "", const int index = 0);
	Array<std::string> GetDefUserIniValues(const NameString& sectionName, const NameString& keyName, Array<std::string> default_values = {});
	void SetIniValue(NameString iniName, const NameString& sectionName, const NameString& keyName, const std::string& newValue, const int index = 0);
	void SetIniValues(NameString iniName, const NameString& sectionName, const NameString& keyName, const Array<std::string>& newValues);
	void SaveAllIniFiles();

	std::string GetVideoFilename(const std::string& name);

	std::string Localize(NameString packageName, const NameString& sectionName, const NameString& keyName, const int index = 0);

	Array<IntObject>& GetIntObjects(const NameString& metaclass);
	const Array<std::string>& GetMaps() const { return maps; }

	bool MissingSESystemIni() const { return missing_se_system_ini; }

	Package* GetTransientPackage() { return GetPackage("Transient"); }

private:
	void CreateTransientPackage();

	std::unique_ptr<IniFile>& LoadIniFile(NameString iniName);
	std::unique_ptr<IniFile>& LoadUserIniFile();
	std::unique_ptr<IniFile>& LoadSystemIniFile();
	fs::path ResolveUserIniPath() const;
	void LoadEngineIniFiles();
	void LoadFileExtensions();
	void LoadIntFiles();
	void LoadLocalizationFilesFromFolder(const fs::path& baseDir, const std::string& localeExt);
	void LoadPackageRemaps();
	std::map<NameString, std::string> ParseIntPublicValue(const std::string& value);

	void ScanForMaps();

	void ScanFolder(const std::string& packagedir, const std::string& search);
	void ScanPaths();
	void ScanSaveInfos();

	void DelayLoadNow();
	void RegisterFunctions();

	void RegisterNativeClasses();

	template<typename T>
	void RegisterNativeClass(const NameString& packageName, const NameString& className, const NameString& baseClass = {});

	std::map<NameString, NativeClass> NativeClasses;

	Array<UObject*> delayLoads;
	int delayLoadActive = 0;

	std::map<NameString, std::string> packageFilenames;
	std::map<NameString, GCRoot<Package>> packages;
	std::map<NameString, std::unique_ptr<IniFile>> iniFiles;
	std::map<NameString, std::shared_ptr<IniFile>> intFiles;
	std::map<std::string, std::string> packageRemaps;
	std::map<NameString, Package*> saveInfos;

	std::map<NameString, std::string> aviFilenames;

	std::map<NameString, std::map<NameString, Array<IntObject>>> IntObjects;

	std::unique_ptr<IniFile> defaultIniFile;  // Holds Default.ini
	std::unique_ptr<IniFile> defaultUserFile; // Holds DefUser.ini

	Array<std::string> mapFolders;
	Array<std::string> maps;

	// So that we don't have to build fs::path()s all over the place
	fs::path gameRootFolderPath;
	fs::path gameSystemFolderPath;
	fs::path gameConfigFolderPath; // Per-user: the SE- ini files
	fs::path gameDataFolderPath;   // Per-user: Save and Export
	fs::path gameSaveFolderPath;
	fs::path gameCacheFolderPath;

	std::string mapExtension;
	std::string saveExtension;
	std::string languageExtension;

	bool missing_se_system_ini = false;

	struct OpenStream
	{
		Package* Pkg = nullptr;
		std::shared_ptr<PackageStream> Stream;
	};

	std::list<OpenStream> openStreams;

	GameLaunchInfo launchInfo;

	friend class Package;
	friend struct SetDelayLoadActive;
};

struct SetDelayLoadActive
{
	SetDelayLoadActive(PackageManager* p) : p(p) { p->delayLoadActive++; }
	~SetDelayLoadActive() { p->delayLoadActive--; }
	PackageManager* p;
};
