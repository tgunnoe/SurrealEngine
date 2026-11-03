#include "Precomp.h"
#include "TestRunner.h"
#include "Package/PackageManager.h"
#include "UnrealURL.h"
#include <chrono>
#include <climits>
#include <fstream>
#include <initializer_list>
#include <utility>
#include <vector>

// These cover the rules that turn a load option or a console argument into a slot, and a slot into
// the files a save lives in. Reading the package and rebuilding the level need a running engine and are
// checked by saving and loading in the three games.

namespace
{
	// A scratch tree under the system temp folder, removed on scope exit so a failing check leaves no debris.
	struct SaveSandbox
	{
		explicit SaveSandbox(const std::string& name)
		{
			Base = fs::temp_directory_path() / ("SurrealTests-saveload-" + std::to_string(TEST_GETPID()) + "-" + name);
			std::error_code ec;
			fs::remove_all(Base, ec);
			fs::create_directories(Base);
		}

		~SaveSandbox()
		{
			std::error_code ec;
			fs::remove_all(Base, ec);
		}

		// Creates a file, with the last write time that many minutes in the past.
		fs::path Touch(const fs::path& relative, int minutesOld = 0) const
		{
			const fs::path path = Base / relative;
			fs::create_directories(path.parent_path());
			std::ofstream(path) << "x";
			fs::last_write_time(path, fs::file_time_type::clock::now() - std::chrono::minutes(minutesOld));
			return path;
		}

		fs::path Base;
	};

	std::string TextOf(const std::optional<std::string>& text)
	{
		return text ? *text : std::string("<nothing>");
	}

	void MakeFolders(const SaveSandbox& sandbox, std::initializer_list<const char*> names)
	{
		for (const char* name : names)
			fs::create_directories(sandbox.Base / name);
	}

	// "1:Save0001 2:Save0002": the whole listing in one text, so that one comparison fails on any slot or name
	std::string SlotsText(const std::vector<std::pair<int, std::string>>& slots)
	{
		std::string text;
		for (const auto& slot : slots)
			text += (text.empty() ? "" : " ") + std::to_string(slot.first) + ":" + slot.second;
		return text;
	}
}

TEST(SaveLoadParseSlotReadsAWholeNumber)
{
	CHECK_EQ(ParseSaveSlot("9").value_or(-99), 9);
	CHECK_EQ(ParseSaveSlot("12").value_or(-99), 12);
	CHECK_EQ(ParseSaveSlot("0").value_or(-99), 0);
	CHECK_EQ(ParseSaveSlot("007").value_or(-99), 7);
	CHECK_EQ(ParseSaveSlot("-1").value_or(99), -1);
	CHECK_EQ(ParseSaveSlot("-12").value_or(99), -12);
	CHECK_EQ(ParseSaveSlot("2147483647").value_or(0), INT_MAX);
	CHECK_EQ(ParseSaveSlot("-2147483648").value_or(0), INT_MIN);
}

TEST(SaveLoadParseSlotRejectsTextThatIsNotAWholeNumber)
{
	// The control: a parser that rejects everything would pass the rest
	CHECK(ParseSaveSlot("9").has_value());

	CHECK(!ParseSaveSlot("9x").has_value());
	CHECK(!ParseSaveSlot("x9").has_value());
	CHECK(!ParseSaveSlot("9 ").has_value());
	CHECK(!ParseSaveSlot(" 9").has_value());
	CHECK(!ParseSaveSlot("9.5").has_value());
	CHECK(!ParseSaveSlot("0x10").has_value());
	CHECK(!ParseSaveSlot("abc").has_value());
	CHECK(!ParseSaveSlot("").has_value());
	CHECK(!ParseSaveSlot("-").has_value());
	CHECK(!ParseSaveSlot("+9").has_value());
	CHECK(!ParseSaveSlot("--1").has_value());
}

TEST(SaveLoadParseSlotRejectsANumberOutsideIntRange)
{
	CHECK(ParseSaveSlot("2147483647").has_value());
	CHECK(ParseSaveSlot("-2147483648").has_value());

	CHECK(!ParseSaveSlot("2147483648").has_value());
	CHECK(!ParseSaveSlot("-2147483649").has_value());
	CHECK(!ParseSaveSlot("99999999999999999999").has_value());
}

TEST(SaveLoadOptionReadsLoad)
{
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("?load=9"))), std::string("9"));
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("Vortex2?Name=Player?load=12"))), std::string("12"));
}

TEST(SaveLoadOptionReadsLoadgame)
{
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("?loadgame=3"))), std::string("3"));
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("?Name=JC?LoadGame=-1"))), std::string("-1"));
}

TEST(SaveLoadOptionPrefersLoadOverLoadgame)
{
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("?loadgame=2?load=1"))), std::string("1"));
}

TEST(SaveLoadOptionIsAbsentWithoutEitherName)
{
	// The control: a reader that finds nothing would pass the rest
	CHECK(LoadOptionText(UnrealURL("?load=9")).has_value());

	CHECK(!LoadOptionText(UnrealURL("?restart")).has_value());
	CHECK(!LoadOptionText(UnrealURL("Vortex2?Name=Player")).has_value());
	// An option that only starts with the name is another option
	CHECK(!LoadOptionText(UnrealURL("?loadgames=3")).has_value());
	CHECK(!LoadOptionText(UnrealURL("?loading=3")).has_value());
	CHECK(!LoadOptionText(UnrealURL("?load2=3")).has_value());
}

TEST(SaveLoadOptionWithoutAValueHasEmptyText)
{
	// Present but not a number, which is not the same as absent
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("?load"))), std::string(""));
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("?loadgame"))), std::string(""));
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("?load="))), std::string(""));
}

TEST(SaveLoadDeusExFolderNamePadsToFourDigits)
{
	CHECK_EQ(DeusExSaveFolderName(1), std::string("Save0001"));
	CHECK_EQ(DeusExSaveFolderName(12), std::string("Save0012"));
	CHECK_EQ(DeusExSaveFolderName(0), std::string("Save0000"));
	CHECK_EQ(DeusExSaveFolderName(9999), std::string("Save9999"));
}

TEST(SaveLoadDeusExFolderNameKeepsAWiderSlotWhole)
{
	CHECK_EQ(DeusExSaveFolderName(10000), std::string("Save10000"));
	CHECK_EQ(DeusExSaveFolderName(123456), std::string("Save123456"));
}

TEST(SaveLoadDeusExFolderNameOfTheQuickSaveSlot)
{
	// The scripts' quick-save slot. The original game writes Save/QuickSave/ for it; the folder a previous
	// build wrote under Save00-1 is no longer looked up.
	CHECK_EQ(DeusExSaveFolderName(-1), std::string("QuickSave"));
}

TEST(SaveLoadSlotPathOfDeusExIsTheSlotFolder)
{
	const fs::path saves = fs::path("userdir") / "Save";
	CHECK_EQ(SaveSlotPath(true, 1, saves, "dxs"), saves / "Save0001");
	CHECK_EQ(SaveSlotPath(true, 12, saves, "dxs"), saves / "Save0012");
	CHECK_EQ(SaveSlotPath(true, 0, saves, "dxs"), saves / "Save0000");
	CHECK_EQ(SaveSlotPath(true, -1, saves, "dxs"), saves / "QuickSave");
}

TEST(SaveLoadSlotPathOfTheOtherGamesIsTheSaveFile)
{
	const fs::path saves = fs::path("userdir") / "Save";
	CHECK_EQ(SaveSlotPath(false, 1, saves, "usa"), saves / "Save1.usa");
	CHECK_EQ(SaveSlotPath(false, 12, saves, "usa"), saves / "Save12.usa");
	CHECK_EQ(SaveSlotPath(false, 0, saves, "usa"), saves / "Save0.usa");
	CHECK_EQ(SaveSlotPath(false, -1, saves, "usa"), saves / "Save-1.usa");
	// The extension is the game's: Unreal Tournament saves as usa too, other games differ
	CHECK_EQ(SaveSlotPath(false, 9, saves, "uas"), saves / "Save9.uas");
}

TEST(SaveLoadSlotRefusalOfDeusExIsBelowTheQuickSaveSlot)
{
	CHECK_EQ(TextOf(SaveSlotRefusal(true, -2)), std::string("slot -2 is not allowed"));
	CHECK_EQ(TextOf(SaveSlotRefusal(true, INT_MIN)), std::string("slot -2147483648 is not allowed"));

	// The controls: the quick-save slot, the scripts' new-save slot and an ordinary slot are allowed
	CHECK(!SaveSlotRefusal(true, -1));
	CHECK(!SaveSlotRefusal(true, 0));
	CHECK(!SaveSlotRefusal(true, 1));
	CHECK(!SaveSlotRefusal(true, INT_MAX));
}

TEST(SaveLoadSlotRefusalOfTheOtherGamesIsBelowZero)
{
	CHECK_EQ(TextOf(SaveSlotRefusal(false, -1)), std::string("slot -1 is not allowed"));
	CHECK_EQ(TextOf(SaveSlotRefusal(false, -2)), std::string("slot -2 is not allowed"));
	CHECK_EQ(TextOf(SaveSlotRefusal(false, INT_MIN)), std::string("slot -2147483648 is not allowed"));

	// The controls: slot 0 and the slots above it are allowed
	CHECK(!SaveSlotRefusal(false, 0));
	CHECK(!SaveSlotRefusal(false, 1));
	CHECK(!SaveSlotRefusal(false, INT_MAX));
}

TEST(SaveLoadNextFreeIndexIsTheLowestMissingFolder)
{
	SaveSandbox sandbox("nextfree");
	const fs::path saves = sandbox.Base / "Save";

	// Before the first save the folder does not exist at all
	CHECK_EQ(NextFreeDeusExSaveIndex(saves), 1);

	fs::create_directories(saves);
	CHECK_EQ(NextFreeDeusExSaveIndex(saves), 1);

	fs::create_directories(saves / "Save0001");
	CHECK_EQ(NextFreeDeusExSaveIndex(saves), 2);

	fs::create_directories(saves / "Save0002");
	CHECK_EQ(NextFreeDeusExSaveIndex(saves), 3);

	// A hole is filled before the count goes on
	fs::remove_all(saves / "Save0001");
	CHECK_EQ(NextFreeDeusExSaveIndex(saves), 1);
}

TEST(SaveLoadNextFreeIndexTreatsAFileAsFree)
{
	SaveSandbox sandbox("nextfreefile");
	const fs::path saves = sandbox.Base / "Save";
	fs::create_directories(saves / "Save0001");
	CHECK_EQ(NextFreeDeusExSaveIndex(saves), 2);

	// Only a folder is a slot: a file in its place does not hold it
	fs::remove_all(saves / "Save0001");
	sandbox.Touch("Save/Save0001");
	CHECK_EQ(NextFreeDeusExSaveIndex(saves), 1);
}

TEST(SaveLoadNewestLevelIsTheNewestFileWithTheSaveExtension)
{
	SaveSandbox sandbox("newest");
	const fs::path slot = sandbox.Base / "Save0001";
	sandbox.Touch("Save0001/01_NYC_Street.dxs", 30);
	sandbox.Touch("Save0001/01_NYC_UNATCOIsland.dxs", 10);

	auto found = NewestDeusExSaveLevel(slot, "dxs");
	CHECK(found.has_value());
	CHECK_EQ(found->filename().string(), std::string("01_NYC_UNATCOIsland.dxs"));
}

TEST(SaveLoadNewestLevelSkipsTheSaveInfoAndBackups)
{
	SaveSandbox sandbox("newestskip");
	const fs::path slot = sandbox.Base / "Save0001";
	sandbox.Touch("Save0001/01_NYC_UNATCOIsland.dxs", 30);
	// All newer than the level, and none of them a level
	sandbox.Touch("Save0001/SaveInfo.dxs", 5);
	sandbox.Touch("Save0001/01_NYC_UNATCOIsland.dxs.old", 4);
	sandbox.Touch("Save0001/01_NYC_UNATCOIsland.tmp", 3);
	sandbox.Touch("Save0001/notes.txt", 2);

	auto found = NewestDeusExSaveLevel(slot, "dxs");
	CHECK(found.has_value());
	CHECK_EQ(found->filename().string(), std::string("01_NYC_UNATCOIsland.dxs"));
	CHECK_EQ(found->parent_path().string(), slot.string());
}

TEST(SaveLoadNewestLevelIsNothingWithoutALevelFile)
{
	SaveSandbox sandbox("newestnone");

	// The control: a finder that finds nothing would pass the rest
	sandbox.Touch("Save0001/01_NYC_UNATCOIsland.dxs");
	CHECK(NewestDeusExSaveLevel(sandbox.Base / "Save0001", "dxs").has_value());

	CHECK(!NewestDeusExSaveLevel(sandbox.Base / "Save0002", "dxs").has_value());

	fs::create_directories(sandbox.Base / "Save0003");
	CHECK(!NewestDeusExSaveLevel(sandbox.Base / "Save0003", "dxs").has_value());

	sandbox.Touch("Save0004/SaveInfo.dxs");
	CHECK(!NewestDeusExSaveLevel(sandbox.Base / "Save0004", "dxs").has_value());

	sandbox.Touch("Save0005/SaveInfo.dxs");
	sandbox.Touch("Save0005/01_NYC_UNATCOIsland.dxs.old");
	CHECK(!NewestDeusExSaveLevel(sandbox.Base / "Save0005", "dxs").has_value());
}

TEST(SaveDirectoryListsTheSlotFoldersInSlotOrder)
{
	SaveSandbox sandbox("listorder");
	MakeFolders(sandbox, { "Save0001", "Save0003", "Save0002" });

	CHECK_EQ(SlotsText(ListDeusExSaveSlots(sandbox.Base)), "1:Save0001 2:Save0002 3:Save0003");
}

TEST(SaveDirectoryLeavesOutWhatIsNotASlotFolder)
{
	SaveSandbox sandbox("listexcludes");
	MakeFolders(sandbox, { "Save0002", "QuickSave", "Save00-1", "Save0000", "Save1", "Save0002x", "Save99999999999", "save0005", "Sav", "ab" });
	sandbox.Touch("notes.txt");
	// A file with a slot's name is not a slot
	sandbox.Touch("Save0004");

	CHECK_EQ(SlotsText(ListDeusExSaveSlots(sandbox.Base)), "2:Save0002");
}

TEST(SaveDirectoryOfAFolderThatIsNotThereIsEmpty)
{
	SaveSandbox sandbox("listabsent");
	MakeFolders(sandbox, { "Save0001" });

	CHECK(ListDeusExSaveSlots(sandbox.Base / "Save").empty());

	// The control: a listing that is always empty would pass the check above
	CHECK_EQ(SlotsText(ListDeusExSaveSlots(sandbox.Base)), "1:Save0001");
}

TEST(SaveDeleteRefusesTheQuickSaveAndEverySlotBelowIt)
{
	for (int slot : { -2, -1, 0 })
		CHECK_EQ(TextOf(SaveSlotDeleteRefusal(slot)), "slot " + std::to_string(slot) + " is not allowed");

	CHECK(!SaveSlotDeleteRefusal(1).has_value());
	CHECK(!SaveSlotDeleteRefusal(9999).has_value());

	// The folder of the quick save is the one a careless remover would take for slot -1
	SaveSandbox sandbox("deletequick");
	MakeFolders(sandbox, { "QuickSave", "Save0001" });
	CHECK_EQ(TextOf(RemoveSaveSlot(true, -1, sandbox.Base, "dxs")), "slot -1 is not allowed");
	CHECK(fs::is_directory(sandbox.Base / "QuickSave"));
	CHECK_EQ(TextOf(RemoveSaveSlot(true, 0, sandbox.Base, "dxs")), "slot 0 is not allowed");
	CHECK(fs::is_directory(sandbox.Base / "Save0001"));
}

TEST(SaveDeleteRemovesTheSlotFolderAndNothingElse)
{
	SaveSandbox sandbox("deletefolder");
	MakeFolders(sandbox, { "Save0001", "Save0003", "QuickSave" });
	sandbox.Touch("Save0002/SaveInfo.dxs");
	sandbox.Touch("Save0002/01_NYC_UNATCOIsland.dxs");

	CHECK_EQ(TextOf(RemoveSaveSlot(true, 2, sandbox.Base, "dxs")), "<nothing>");

	CHECK(!fs::exists(sandbox.Base / "Save0002"));
	CHECK(fs::is_directory(sandbox.Base / "Save0001"));
	CHECK(fs::is_directory(sandbox.Base / "Save0003"));
	CHECK(fs::is_directory(sandbox.Base / "QuickSave"));
}

TEST(SaveDeleteReportsASlotThatHasNoSave)
{
	SaveSandbox sandbox("deleteabsent");
	MakeFolders(sandbox, { "Save0001" });
	// Not a slot: a Deus Ex slot is a folder
	sandbox.Touch("Save0004");

	CHECK_EQ(TextOf(RemoveSaveSlot(true, 5, sandbox.Base, "dxs")), "no save in slot 5");
	CHECK_EQ(TextOf(RemoveSaveSlot(true, 4, sandbox.Base, "dxs")), "no save in slot 4");
	CHECK(fs::is_regular_file(sandbox.Base / "Save0004"));
	CHECK_EQ(TextOf(RemoveSaveSlot(true, 1, sandbox.Base / "NotThere", "dxs")), "no save in slot 1");
	CHECK(fs::is_directory(sandbox.Base / "Save0001"));
}

TEST(SaveDeleteRemovesTheSaveFileInTheOtherGames)
{
	SaveSandbox sandbox("deletefile");
	sandbox.Touch("Save3.usa");
	sandbox.Touch("Save4.usa");

	CHECK_EQ(TextOf(RemoveSaveSlot(false, 3, sandbox.Base, "usa")), "<nothing>");

	CHECK(!fs::exists(sandbox.Base / "Save3.usa"));
	CHECK(fs::exists(sandbox.Base / "Save4.usa"));
	CHECK_EQ(TextOf(RemoveSaveSlot(false, 3, sandbox.Base, "usa")), "no save in slot 3");
}

TEST(SaveDeleteRefusesASlotThatResolvesOutsideTheSaveFolder)
{
	SaveSandbox sandbox("deleteoutside");
	sandbox.Touch("elsewhere/keep.txt");
	fs::create_directories(sandbox.Base / "Save");

	std::error_code error;
	fs::create_directory_symlink(sandbox.Base / "elsewhere", sandbox.Base / "Save" / "Save0007", error);
	if (error)
		return; // A platform that does not let this user make links has no such slot to refuse

	CHECK_EQ(TextOf(RemoveSaveSlot(true, 7, sandbox.Base / "Save", "dxs")), "slot 7 is outside the save folder");
	CHECK(fs::exists(sandbox.Base / "elsewhere" / "keep.txt"));
	CHECK(fs::is_symlink(sandbox.Base / "Save" / "Save0007"));

	// A slot that is the save folder itself would take every other slot with it
	fs::create_directory_symlink(sandbox.Base / "Save", sandbox.Base / "Save" / "Save0008", error);
	CHECK_EQ(TextOf(RemoveSaveSlot(true, 8, sandbox.Base / "Save", "dxs")), "slot 8 is outside the save folder");
	CHECK(fs::is_symlink(sandbox.Base / "Save" / "Save0007"));
}
