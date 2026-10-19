
#include "Precomp.h"
#include "UGameDirectory.h"
#include "Utils/Logger.h"
#include "Utils/File.h"
#include "Engine.h"
#include "Package/PackageManager.h"
#include <algorithm>
#include <vector>

namespace
{
	// The save info a folder's package holds, or null when the package manager has none for it
	UDXSaveInfo* SaveInfoOfFolder(const std::string& folderName)
	{
		auto pkg = engine->packages->GetSaveInfoPackage(folderName);

		if (!pkg)
			return nullptr;

		return UObject::Cast<UDXSaveInfo>(pkg->GetUObject("DeusExSaveInfo", "MyDeusExSaveInfo"));
	}
}

void UDXGameDirectory::GetGameDirectory()
{
	// The player keeps one directory object and its scripts never free it, so a new listing replaces
	// what the last one left
	DirectoryList().Array->Clear();
	LoadedSaveInfoPointers().Array->Clear();

	if (GameDirectoryType() == EGameDirectoryTypes::GD_Maps)
	{
		currentDirectory = fs::path(engine->LaunchInfo.gameRootFolder) / "Maps";
		PopulateDirectoryList();
	}
	else
	{
		currentDirectory = engine->packages->GetSaveFolderPath();
		PopulateSaveInfoPointers();
	}
}

int UDXGameDirectory::GetNewSaveFileIndex()
{
	return NextFreeDeusExSaveIndex(engine->packages->GetSaveFolderPath());
}

std::string UDXGameDirectory::GenerateSaveFilename(int saveIndex)
{
	return DeusExSaveFolderName(saveIndex);
}

std::string UDXGameDirectory::GenerateNewSaveFileName(std::optional<int> newIndex)
{
	return GenerateSaveFilename(newIndex ? *newIndex : GetNewSaveFileIndex());
}

int UDXGameDirectory::GetDirCount()
{
	return static_cast<int>(DirectoryList().size());
}

std::string UDXGameDirectory::GetDirFilename(int fileIndex)
{
	auto names = DirectoryList();
	if (fileIndex < 0 || static_cast<size_t>(fileIndex) >= names.size())
		return {};

	return names[fileIndex];
}

void UDXGameDirectory::SetDirType(EGameDirectoryTypes newDirType)
{
	GameDirectoryType() = newDirType;
}

void UDXGameDirectory::SetDirFilter(const std::string& strFilter)
{
	CurrentFilter() = strFilter;
}

UDXSaveInfo* UDXGameDirectory::GetSaveInfo(int fileIndex)
{
	if (GameDirectoryType() != EGameDirectoryTypes::GD_SaveGames)
		// We're not in the Save folder
		return nullptr;

	return SaveInfoOfFolder(DeusExSaveFolderName(fileIndex));
}

UDXSaveInfo* UDXGameDirectory::GetSaveInfoFromDirectoryIndex(int DirectoryIndex)
{
	// The scripts pass the position in the listing and release each save info they were given inside
	// their loop, so the folder's name is looked up rather than a position in the loaded pointers,
	// which shrink as they are released
	auto names = DirectoryList();
	if (GameDirectoryType() != EGameDirectoryTypes::GD_SaveGames || DirectoryIndex < 0 || static_cast<size_t>(DirectoryIndex) >= names.size())
		return nullptr;

	return SaveInfoOfFolder(names[DirectoryIndex]);
}

UDXSaveInfo* UDXGameDirectory::GetTempSaveInfo()
{
	// The save screen fills this in for a game that has no slot yet, and nothing is ever saved from it
	if (!TempSaveInfo())
	{
		auto cls = engine->packages->FindClass("DeusEx.DeusExSaveInfo");
		TempSaveInfo() = Cast<UDXSaveInfo>(engine->packages->GetTransientPackage()->NewObject("TempSaveInfo", cls, ObjectFlags::Transient));
	}

	return TempSaveInfo();
}

void UDXGameDirectory::DeleteSaveInfo(UDXSaveInfo& saveInfo)
{
	// The scripts call this to release each save info they add to a screen, so it only forgets the entry:
	// the slot's folder is deleted by the console command of the screens' delete button
	auto pointers = LoadedSaveInfoPointers();
	for (size_t i = 0; i < pointers.size(); i++)
	{
		if (pointers[i] == &saveInfo)
		{
			pointers.Array->Remove(i, 1);
			return;
		}
	}
}

void UDXGameDirectory::PurgeAllSaveInfo()
{
	if (GameDirectoryType() == EGameDirectoryTypes::GD_Maps)
		return;

	// Scripts can reach this with any directory selected; only the player's own save folder may be emptied
	if (currentDirectory != engine->packages->GetSaveFolderPath())
	{
		LogMessage("PurgeAllSaveInfo ignored: the selected directory is not the save folder");
		return;
	}

	if (!engine->LaunchInfo.CanWriteUserFiles())
	{
		LogMessage("PurgeAllSaveInfo skipped: --autostart without --user-dir does not change saves");
		return;
	}

	if (!fs::is_directory(currentDirectory))
		return;

	// Collected first so that removing entries cannot disturb the iteration
	Array<std::string> entries;
	for (auto& p : fs::directory_iterator(currentDirectory))
		entries.push_back(p.path().string());

	// Save slots are folders; the Save folder itself stays
	for (const std::string& entry : entries)
		Directory::remove_all(entry);
}

int UDXGameDirectory::GetSaveFreeSpace()
{
	// Returns a value in KBs, which limits us to ~2TB of "free space" max.
	// Should be enough but still
	// The save folder does not exist until the first save; the space that matters is that of the nearest folder that does
	fs::path folder = currentDirectory;
	while (folder.has_parent_path() && folder != folder.parent_path() && !fs::exists(folder))
		folder = folder.parent_path();

	const auto freeSpaceInKBs = static_cast<int>(fs::space(folder).free / 1024);
	// Capped at 1TB
	return std::min(freeSpaceInKBs, 1000 * 1024 * 1024);
}

int UDXGameDirectory::GetSaveDirectorySize(int saveIndex)
{
	if (GameDirectoryType() != EGameDirectoryTypes::GD_SaveGames)
		// We're not in the Save folder
		return 0;

	int size = 0;

	for (auto& p : fs::directory_iterator(currentDirectory / DeusExSaveFolderName(saveIndex)))
		size += (int)p.file_size();

	return size;
}

void UDXGameDirectory::PopulateDirectoryList()
{
	// Sorted, as the original game's directory listing is
	std::vector<std::string> names;

	std::error_code error;
	for (fs::directory_iterator it{ currentDirectory, error }, end; !error && it != end; it.increment(error))
	{
		std::error_code entryError;
		if (it->is_regular_file(entryError))
			names.push_back(it->path().filename().string());
	}

	std::sort(names.begin(), names.end());
	for (const std::string& name : names)
		DirectoryList().push_back(name);
}

void UDXGameDirectory::PopulateSaveInfoPointers()
{
	// A folder without a save info stays in the listing, so the screens show it as missing
	for (const auto& slot : ListDeusExSaveSlots(currentDirectory))
	{
		DirectoryList().push_back(slot.second);

		if (UDXSaveInfo* saveInfo = SaveInfoOfFolder(slot.second))
			LoadedSaveInfoPointers().push_back(saveInfo);
	}
}
