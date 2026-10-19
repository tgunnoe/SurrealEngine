#include "Precomp.h"
#include "TestRunner.h"
#include "Package/PackageManager.h"
#include "UnrealURL.h"
#include <chrono>
#include <climits>
#include <fstream>

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

	// A URL with several options, built option by option: the string parser drops the last option when it
	// is given more than one, so such a URL cannot be written as one string here.
	UnrealURL UrlWith(const std::string& map, std::initializer_list<const char*> options)
	{
		UnrealURL url(map);
		for (const char* option : options)
			url.AddOrReplaceOption(option);
		return url;
	}

	std::string TextOf(const std::optional<std::string>& text)
	{
		return text ? *text : std::string("<nothing>");
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
	CHECK_EQ(TextOf(LoadOptionText(UrlWith("Vortex2", { "Name=Player", "load=12" }))), std::string("12"));
}

TEST(SaveLoadOptionReadsLoadgame)
{
	CHECK_EQ(TextOf(LoadOptionText(UnrealURL("?loadgame=3"))), std::string("3"));
	CHECK_EQ(TextOf(LoadOptionText(UrlWith("", { "Name=JC", "LoadGame=-1" }))), std::string("-1"));
}

TEST(SaveLoadOptionPrefersLoadOverLoadgame)
{
	CHECK_EQ(TextOf(LoadOptionText(UrlWith("", { "loadgame=2", "load=1" }))), std::string("1"));
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
