#include "Precomp.h"
#include "TestRunner.h"
#include "GameFolder.h"
#include "Utils/File.h"
#include "Utils/UserDirs.h"
#include <filesystem>
#include <map>
#include <string>

#ifndef WIN32
#include <unistd.h>
#endif

namespace
{
	using Vars = std::map<std::string, std::string>;

	// An environment that is exactly the given table, whatever the host has set.
	UserDirs::EnvLookup Env(Vars vars)
	{
		return [vars](const char* name) -> const char*
		{
			auto it = vars.find(name);
			return it == vars.end() ? nullptr : it->second.c_str();
		};
	}

	GameLaunchInfo Install(const std::string& exe, const std::string& version, const std::string& root)
	{
		GameLaunchInfo info;
		info.gameExecutableName = exe;
		info.gameVersionString = version;
		info.gameRootFolder = root;
		return info;
	}

	// What WritePolicy::check says about a path: empty when it is allowed, the exception text when refused.
	std::string Refusal(const std::filesystem::path& path)
	{
		try
		{
			WritePolicy::check(path);
			return {};
		}
		catch (const std::exception& e)
		{
			return e.what();
		}
	}

	// The same for WritePolicy::checkAgainst
	std::string RefusalAgainst(const std::filesystem::path& folder, const std::filesystem::path& path)
	{
		try
		{
			WritePolicy::checkAgainst(folder, path);
			return {};
		}
		catch (const std::exception& e)
		{
			return e.what();
		}
	}

	std::string Refused(const std::filesystem::path& path)
	{
		return "refusing to write inside the game folder: " + path.string();
	}

	// A scratch tree under the system temp folder. The refusal is lifted and the tree removed on scope exit,
	// so a failing check leaves neither a forbidden root nor debris behind.
	struct Sandbox
	{
		explicit Sandbox(const std::string& name)
		{
			Base = std::filesystem::temp_directory_path() / ("SurrealTests-userdirs-" + std::to_string(TEST_GETPID()) + "-" + name);
			std::error_code ec;
			std::filesystem::remove_all(Base, ec);
			std::filesystem::create_directories(Base / "game" / "System");
			std::filesystem::create_directories(Base / "out");
		}

		~Sandbox()
		{
			WritePolicy::clearForbiddenRoot();
			std::error_code ec;
			std::filesystem::remove_all(Base, ec);
		}

		std::filesystem::path Game() const { return Base / "game"; }
		std::filesystem::path Out() const { return Base / "out"; }

		std::filesystem::path Base;
	};
}

#ifndef WIN32

TEST(UserDirsConfigRootHonoursXdgConfigHome)
{
	CHECK_EQ(UserDirs::configRoot(Env({ { "XDG_CONFIG_HOME", "/x/cfg" }, { "HOME", "/h" } })).string(), std::string("/x/cfg"));
}

TEST(UserDirsConfigRootFallsBackToHomeDotConfig)
{
	CHECK_EQ(UserDirs::configRoot(Env({ { "HOME", "/h" } })).string(), std::string("/h/.config"));
}

TEST(UserDirsConfigRootIgnoresEmptyAndRelativeXdgConfigHome)
{
	CHECK_EQ(UserDirs::configRoot(Env({ { "XDG_CONFIG_HOME", "" }, { "HOME", "/h" } })).string(), std::string("/h/.config"));
	CHECK_EQ(UserDirs::configRoot(Env({ { "XDG_CONFIG_HOME", "rel/cfg" }, { "HOME", "/h" } })).string(), std::string("/h/.config"));
}

TEST(UserDirsRootsCarryNoTrailingSeparator)
{
	CHECK_EQ(UserDirs::configRoot(Env({ { "XDG_CONFIG_HOME", "/x/cfg/" } })).string(), std::string("/x/cfg"));
	CHECK_EQ(UserDirs::configRoot(Env({ { "XDG_CONFIG_HOME", "/x/cfg//" } })).string(), std::string("/x/cfg"));
	CHECK_EQ(UserDirs::configRoot(Env({ { "HOME", "/h/" } })).string(), std::string("/h/.config"));
	CHECK_EQ(UserDirs::dataRoot(Env({ { "XDG_DATA_HOME", "/x/data/" } })).string(), std::string("/x/data"));
	CHECK_EQ(UserDirs::dataRoot(Env({ { "HOME", "/h/" } })).string(), std::string("/h/.local/share"));
}

TEST(UserDirsDataRootHonoursXdgDataHome)
{
	CHECK_EQ(UserDirs::dataRoot(Env({ { "XDG_DATA_HOME", "/x/data" }, { "HOME", "/h" } })).string(), std::string("/x/data"));
}

TEST(UserDirsDataRootFallsBackToHomeLocalShare)
{
	CHECK_EQ(UserDirs::dataRoot(Env({ { "HOME", "/h" } })).string(), std::string("/h/.local/share"));
}

TEST(UserDirsDataRootIgnoresEmptyAndRelativeXdgDataHome)
{
	CHECK_EQ(UserDirs::dataRoot(Env({ { "XDG_DATA_HOME", "" }, { "HOME", "/h" } })).string(), std::string("/h/.local/share"));
	CHECK_EQ(UserDirs::dataRoot(Env({ { "XDG_DATA_HOME", "rel/data" }, { "HOME", "/h" } })).string(), std::string("/h/.local/share"));
}

TEST(UserDirsRootsWithoutHomeFallBackToPerUserTempDirectory)
{
	const std::string expected = (std::filesystem::temp_directory_path() / ("SurrealEngine-" + std::to_string(getuid()))).string();

	// Neither XDG variable is usable and HOME is unset, empty or relative: a path, not an exception
	CHECK_EQ(UserDirs::configRoot(Env({})).string(), expected);
	CHECK_EQ(UserDirs::dataRoot(Env({})).string(), expected);
	CHECK_EQ(UserDirs::configRoot(Env({ { "HOME", "" } })).string(), expected);
	CHECK_EQ(UserDirs::dataRoot(Env({ { "HOME", "relative/home" } })).string(), expected);
}

#endif

TEST(UserDirsGameKeyForTheThreeTargetInstalls)
{
	CHECK_EQ(UserDirs::gameKey(Install("DeusEx", "1112fm", "/home/u/Games/gog/gmdx/drive_c/GOG Games/GMDX")), std::string("DeusEx-1112fm-GMDX"));
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/home/u/Games/gog/unreal-gold/drive_c/GOG Games/Unreal Gold")), std::string("Unreal-226b-Unreal_Gold"));
	CHECK_EQ(UserDirs::gameKey(Install("UnrealTournament", "436", "/home/u/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY")), std::string("UnrealTournament-436-Unreal_Tournament_GOTY"));
}

TEST(UserDirsGameKeyIgnoresTrailingSeparatorsAndDotSegments)
{
	const std::string expected = "Unreal-226b-Unreal_Gold";
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/g/Unreal Gold/")), expected);
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/g/Unreal Gold//")), expected);
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/g/x/../Unreal Gold")), expected);
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/g/./Unreal Gold")), expected);
}

TEST(UserDirsGameKeyReplacesCharactersOutsideTheAllowedSet)
{
	CHECK_EQ(UserDirs::gameKey(Install("DeusEx", "1112fm", "/g/My Game (v1.2) [x]!")), std::string("DeusEx-1112fm-My_Game__v1.2___x__"));
	CHECK_EQ(UserDirs::gameKey(Install("DeusEx", "1112fm", "/g/a-b_c.d")), std::string("DeusEx-1112fm-a-b_c.d"));
	// One underscore per character, not per byte
	CHECK_EQ(UserDirs::gameKey(Install("DeusEx", "1112fm", "/g/Caf\xC3\xA9")), std::string("DeusEx-1112fm-Caf_"));
}

TEST(UserDirsGameKeyCutsALongLeafTo32Characters)
{
	const std::string leaf40 = "abcdefghijklmnopqrstuvwxyz0123456789ABCD";
	CHECK_EQ(leaf40.size(), (size_t)40);
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/g/" + leaf40)), std::string("Unreal-226b-abcdefghijklmnopqrstuvwxyz012345"));
	// Exactly 32 stays whole
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/g/abcdefghijklmnopqrstuvwxyz012345")), std::string("Unreal-226b-abcdefghijklmnopqrstuvwxyz012345"));
}

TEST(UserDirsGameKeyUsesRootForAnEmptyLeaf)
{
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "")), std::string("Unreal-226b-root"));
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/")), std::string("Unreal-226b-root"));
}

TEST(UserDirsGameKeyDependsOnExecutableVersionAndLeafOnly)
{
	const std::string key = UserDirs::gameKey(Install("Unreal", "226b", "/a/Unreal Gold"));

	// A different parent folder is the same install as far as the key goes
	CHECK_EQ(UserDirs::gameKey(Install("Unreal", "226b", "/b/c/Unreal Gold")), key);
	// A mod install of the same version in a differently named folder gets its own directory
	CHECK(UserDirs::gameKey(Install("Unreal", "226b", "/a/Unreal Gold Mod")) != key);
	CHECK(UserDirs::gameKey(Install("Unreal", "227i", "/a/Unreal Gold")) != key);
	CHECK(UserDirs::gameKey(Install("UnrealTournament", "226b", "/a/Unreal Gold")) != key);
}

#ifndef WIN32

TEST(UserDirsGameDirsLiveUnderTheRootsAndTheKey)
{
	const GameLaunchInfo info = Install("DeusEx", "1112fm", "/g/GMDX");
	const UserDirs::GameDirs dirs = UserDirs::gameDirs(info, Env({ { "XDG_CONFIG_HOME", "/c" }, { "XDG_DATA_HOME", "/d" } }));

	CHECK_EQ(dirs.config.string(), std::string("/c/SurrealEngine/DeusEx-1112fm-GMDX"));
	CHECK_EQ(dirs.data.string(), std::string("/d/SurrealEngine/DeusEx-1112fm-GMDX"));
}

TEST(UserDirsGameDirsAreTheUserDirWhenOneWasGiven)
{
	GameLaunchInfo info = Install("DeusEx", "1112fm", "/g/GMDX");
	info.userDir = "/u/run1";
	const UserDirs::GameDirs dirs = UserDirs::gameDirs(info, Env({ { "XDG_CONFIG_HOME", "/c" }, { "XDG_DATA_HOME", "/d" } }));

	CHECK_EQ(dirs.config.string(), std::string("/u/run1"));
	CHECK_EQ(dirs.data.string(), std::string("/u/run1"));
}

TEST(UserDirsGameDirsResolveARelativeInstallFolderAgainstTheCurrentDirectory)
{
	const std::filesystem::path cwd = std::filesystem::current_path();
	const GameLaunchInfo here = Install("Unreal", "226b", ".");
	const GameLaunchInfo named = Install("Unreal", "226b", cwd.string());
	const UserDirs::EnvLookup env = Env({ { "XDG_CONFIG_HOME", "/c" }, { "XDG_DATA_HOME", "/d" } });

	CHECK(!UserDirs::gameKey(named).empty());
	CHECK_EQ(UserDirs::gameDirs(here, env).config.string(), "/c/SurrealEngine/" + UserDirs::gameKey(named));
	CHECK_EQ(UserDirs::gameDirs(named, env).config.string(), "/c/SurrealEngine/" + UserDirs::gameKey(named));
}

#endif

TEST(UserDirsResolveUserDirArgIsAbsoluteAndNormalized)
{
	const std::filesystem::path cwd = std::filesystem::current_path();

	CHECK((UserDirs::resolveUserDirArg("runs/one")).is_absolute());
	CHECK_EQ(UserDirs::resolveUserDirArg("runs/one").string(), std::filesystem::weakly_canonical(cwd / "runs" / "one").string());
	CHECK_EQ(UserDirs::resolveUserDirArg("runs/two/../one").string(), UserDirs::resolveUserDirArg("runs/one").string());
	CHECK_EQ(UserDirs::resolveUserDirArg(cwd.string()).string(), std::filesystem::weakly_canonical(cwd).string());
	CHECK_THROWS(UserDirs::resolveUserDirArg(""));
}

TEST(UserDirsPolicyAllowsEverythingBeforeARootIsSet)
{
	Sandbox box("before");
	WritePolicy::clearForbiddenRoot();

	CHECK_EQ(Refusal(box.Game()), std::string());
	CHECK_EQ(Refusal(box.Game() / "System" / "SE-Unreal.ini"), std::string());
	CHECK_EQ(Refusal(box.Out() / "x"), std::string());
}

TEST(UserDirsPolicyRefusesTheRootAndEverythingBelowIt)
{
	Sandbox box("inside");
	const std::filesystem::path root = box.Base / "g" / "Unreal";
	WritePolicy::setForbiddenRoot(root);

	for (const std::filesystem::path& p : { root, root / "System", root / "System" / "SE-Unreal.ini", root / "a" / "b" / "c" })
		CHECK_EQ(Refusal(p), Refused(p));
}

TEST(UserDirsPolicyAllowsOutsidePathsAndPrefixSiblings)
{
	Sandbox box("outside");
	const std::filesystem::path root = box.Base / "g" / "Unreal";
	WritePolicy::setForbiddenRoot(root);

	// "/g/Unreal" must not cover "/g/UnrealGold": the prefix is per component, not per character
	for (const std::filesystem::path& p : { root.parent_path(), root.parent_path() / "UnrealGold", root.parent_path() / "UnrealGold" / "x", root.parent_path() / "Unreal.bak", box.Base / "other" / "Unreal" / "x", box.Out() / "x" })
		CHECK_EQ(Refusal(p), std::string());
}

TEST(UserDirsPolicyIgnoresTrailingSeparators)
{
	Sandbox box("slash");
	const std::filesystem::path root = box.Base / "g" / "Unreal";
	const std::string sep(1, std::filesystem::path::preferred_separator);

	WritePolicy::setForbiddenRoot(root.string() + sep);

	CHECK_EQ(Refusal(root), Refused(root));
	CHECK_EQ(Refusal(root.string() + sep), Refused(root.string() + sep));
	CHECK_EQ(Refusal(root / "x"), Refused(root / "x"));
	CHECK_EQ(Refusal(root.parent_path() / ("UnrealGold" + sep)), std::string());
}

TEST(UserDirsPolicyResolvesDotDotBeforeComparing)
{
	Sandbox box("dotdot");
	const std::filesystem::path root = box.Base / "g" / "Unreal";
	WritePolicy::setForbiddenRoot(root);

	// Leaves the folder, so allowed
	CHECK_EQ(Refusal(root / ".." / "Other" / "x"), std::string());
	// Enters (or stays in) the folder by a detour, so refused
	const std::filesystem::path in1 = root.parent_path() / "Other" / ".." / "Unreal" / "x";
	const std::filesystem::path in2 = root / "sub" / ".." / ".." / "Unreal" / "x";
	const std::filesystem::path in3 = root / ".." / "Unreal";
	CHECK_EQ(Refusal(in1), Refused(in1));
	CHECK_EQ(Refusal(in2), Refused(in2));
	CHECK_EQ(Refusal(in3), Refused(in3));
}

TEST(UserDirsPolicyFollowsRelativePathsFromTheCurrentDirectory)
{
	Sandbox box("relative");
	WritePolicy::setForbiddenRoot(box.Game());
	const std::filesystem::path before = std::filesystem::current_path();

	std::filesystem::current_path(box.Game() / "System");
	const std::string inside = Refusal("SE-Unreal.ini");
	std::filesystem::current_path(box.Out());
	const std::string outside = Refusal("SE-Unreal.ini");
	std::filesystem::current_path(before);

	CHECK_EQ(inside, Refused("SE-Unreal.ini"));
	CHECK_EQ(outside, std::string());
}

#ifndef WIN32

TEST(UserDirsPolicyFollowsSymbolicLinks)
{
	Sandbox box("symlink");
	const std::filesystem::path link = box.Out() / "gamelink";
	std::filesystem::create_directory_symlink(box.Game(), link);

	// A path that reaches the folder through a link is inside it
	WritePolicy::setForbiddenRoot(box.Game());
	CHECK_EQ(Refusal(link / "System" / "x"), Refused(link / "System" / "x"));

	// And a root given as a link guards the real folder
	WritePolicy::setForbiddenRoot(link);
	CHECK_EQ(Refusal(box.Game() / "System" / "x"), Refused(box.Game() / "System" / "x"));
	CHECK_EQ(Refusal(box.Out() / "x"), std::string());
}

#endif

TEST(UserDirsPolicyClearAndReplaceChangeTheForbiddenRoot)
{
	Sandbox box("replace");
	WritePolicy::setForbiddenRoot(box.Game());
	CHECK_EQ(Refusal(box.Game() / "x"), Refused(box.Game() / "x"));

	WritePolicy::setForbiddenRoot(box.Out());
	CHECK_EQ(Refusal(box.Game() / "x"), std::string());
	CHECK_EQ(Refusal(box.Out() / "x"), Refused(box.Out() / "x"));

	WritePolicy::clearForbiddenRoot();
	CHECK_EQ(Refusal(box.Out() / "x"), std::string());
}

TEST(UserDirsPolicyRejectsAnEmptyRoot)
{
	Sandbox box("emptyroot");
	WritePolicy::setForbiddenRoot(box.Game());

	// An empty root would silently switch the guard off; it must fail loudly and leave the old root in force
	CHECK_THROWS(WritePolicy::setForbiddenRoot(std::filesystem::path()));
	CHECK_EQ(Refusal(box.Game() / "x"), Refused(box.Game() / "x"));
}

TEST(UserDirsWrapperCreateAlwaysRefusesInsideTheRoot)
{
	Sandbox box("create");
	WritePolicy::setForbiddenRoot(box.Game());

	const std::filesystem::path inside = box.Game() / "System" / "SE-Unreal.ini";
	CHECK_THROWS(File::create_always(inside.string()));
	CHECK_THROWS(File::write_all_text(inside.string(), "x"));
	CHECK_THROWS(File::write_all_bytes(inside.string(), "x", 1));
	CHECK(!std::filesystem::exists(inside));

	const std::filesystem::path outside = box.Out() / "SE-Unreal.ini";
	File::write_all_text(outside.string(), "x");
	CHECK(std::filesystem::exists(outside));
}

TEST(UserDirsWrapperDirectoryCreateRefusesInsideTheRoot)
{
	Sandbox box("mkdir");
	WritePolicy::setForbiddenRoot(box.Game());

	const std::filesystem::path inside = box.Game() / "Save" / "deeper";
	CHECK_THROWS(Directory::create(inside.string()));
	CHECK(!std::filesystem::exists(box.Game() / "Save"));

	Directory::create((box.Out() / "Save" / "deeper").string());
	CHECK(std::filesystem::is_directory(box.Out() / "Save" / "deeper"));
}

TEST(UserDirsWrapperTryDeleteRefusesInsideTheRoot)
{
	Sandbox box("delete");
	const std::filesystem::path inside = box.Game() / "System" / "User.ini";
	const std::filesystem::path outside = box.Out() / "User.ini";
	File::write_all_text(inside.string(), "x");
	File::write_all_text(outside.string(), "x");
	WritePolicy::setForbiddenRoot(box.Game());

	CHECK_THROWS(File::try_delete(inside.string()));
	CHECK_THROWS(File::delete_always(inside.string()));
	CHECK(std::filesystem::exists(inside));

	CHECK(File::try_delete(outside.string()));
	CHECK(!std::filesystem::exists(outside));
}

TEST(UserDirsWrapperRenameRefusesWhenEitherEndIsInsideTheRoot)
{
	Sandbox box("rename");
	const std::filesystem::path insideFile = box.Game() / "System" / "a.tmp";
	const std::filesystem::path outsideFile = box.Out() / "b.tmp";
	File::write_all_text(insideFile.string(), "x");
	File::write_all_text(outsideFile.string(), "y");
	WritePolicy::setForbiddenRoot(box.Game());

	// Moving a file out of the folder removes it from there; moving one in creates it there
	CHECK_THROWS(File::rename(insideFile.string(), (box.Out() / "moved").string()));
	CHECK(std::filesystem::exists(insideFile));
	CHECK_THROWS(File::rename(outsideFile.string(), (box.Game() / "System" / "moved").string()));
	CHECK(std::filesystem::exists(outsideFile));
	CHECK(!std::filesystem::exists(box.Game() / "System" / "moved"));

	File::rename(outsideFile.string(), (box.Out() / "c.tmp").string());
	CHECK(std::filesystem::exists(box.Out() / "c.tmp"));
}

TEST(UserDirsWrapperRemoveAllRefusesInsideTheRoot)
{
	Sandbox box("removeall");
	const std::filesystem::path insideDir = box.Game() / "Save" / "Save0001";
	const std::filesystem::path outsideDir = box.Out() / "Save" / "Save0001";
	std::filesystem::create_directories(insideDir);
	std::filesystem::create_directories(outsideDir);
	WritePolicy::setForbiddenRoot(box.Game());

	CHECK_THROWS(Directory::remove_all(insideDir.string()));
	CHECK_THROWS(Directory::remove_all((box.Game() / "Save").string()));
	CHECK(std::filesystem::is_directory(insideDir));

	Directory::remove_all(outsideDir.string());
	CHECK(!std::filesystem::exists(outsideDir));
}

TEST(UserDirsCheckAgainstRefusesTheFolderAndEverythingBelowIt)
{
	Sandbox box("against");
	WritePolicy::clearForbiddenRoot();
	const std::filesystem::path folder = box.Game();
	const std::string sep(1, std::filesystem::path::preferred_separator);

	// No stored root is needed: the caller names the folder
	for (const std::filesystem::path& p : { folder, folder / "System", folder / "System" / "x.log", folder / "System" / ".." / "x.log", folder.parent_path() / "game" / "a" / ".." / "b" })
		CHECK_EQ(RefusalAgainst(folder, p), Refused(p));

	// A trailing separator on the folder or on the path changes nothing
	CHECK_EQ(RefusalAgainst(folder.string() + sep, folder / "x.log"), Refused(folder / "x.log"));
	CHECK_EQ(RefusalAgainst(folder, folder.string() + sep), Refused(folder.string() + sep));
}

TEST(UserDirsCheckAgainstAllowsOutsidePathsAndPrefixSiblings)
{
	Sandbox box("againstout");
	const std::filesystem::path folder = box.Base / "g" / "Unreal";

	for (const std::filesystem::path& p : { folder.parent_path(), folder.parent_path() / "UnrealGold" / "x.log", folder.parent_path() / "Unreal.bak", box.Out() / "x.log", folder / ".." / "x.log" })
		CHECK_EQ(RefusalAgainst(folder, p), std::string());

	// Nothing to compare: an empty folder or an empty path is not a refusal
	CHECK_EQ(RefusalAgainst(std::filesystem::path(), folder / "x.log"), std::string());
	CHECK_EQ(RefusalAgainst(folder, std::filesystem::path()), std::string());
}

TEST(UserDirsCheckAgainstLeavesTheStoredRootAlone)
{
	Sandbox box("againstroot");
	WritePolicy::setForbiddenRoot(box.Out());

	CHECK_EQ(RefusalAgainst(box.Game(), box.Game() / "x.log"), Refused(box.Game() / "x.log"));

	// The stored root still guards its own folder and still allows the other one
	CHECK_EQ(Refusal(box.Out() / "x"), Refused(box.Out() / "x"));
	CHECK_EQ(Refusal(box.Game() / "x"), std::string());
}

#ifndef WIN32

TEST(UserDirsCheckAgainstFollowsSymbolicLinks)
{
	Sandbox box("againstlink");
	const std::filesystem::path link = box.Out() / "gamelink";
	std::filesystem::create_directory_symlink(box.Game(), link);

	// A log named through a link into the folder is inside it, whichever side the link is on
	CHECK_EQ(RefusalAgainst(box.Game(), link / "System" / "x.log"), Refused(link / "System" / "x.log"));
	CHECK_EQ(RefusalAgainst(link, box.Game() / "System" / "x.log"), Refused(box.Game() / "System" / "x.log"));
}

#endif

TEST(UserDirsUserIniChainOrder)
{
	const std::filesystem::path config = "cfg";
	const std::filesystem::path system = "sys";

	const std::vector<std::filesystem::path> chain = UserDirs::userIniChain(config, system);
	const std::vector<std::filesystem::path> expected = { config / "SE-User.ini", system / "SE-User.ini", system / "User.ini", system / "user.ini", system / "DefUser.ini" };

	CHECK_EQ(chain.size(), expected.size());
	for (size_t i = 0; i < expected.size(); i++)
		CHECK_EQ(chain[i].string(), expected[i].string());
}

TEST(UserDirsFirstExistingFileTakesTheEarliestFile)
{
	Sandbox box("firstexisting");
	const std::filesystem::path config = box.Out();
	const std::filesystem::path system = box.Game() / "System";
	const std::vector<std::filesystem::path> chain = UserDirs::userIniChain(config, system);

	// Nothing exists: the last resort, named so that opening it fails with a useful path
	CHECK_EQ(UserDirs::firstExistingFile(chain).string(), (system / "DefUser.ini").string());

	File::write_all_text((system / "DefUser.ini").string(), "x");
	CHECK_EQ(UserDirs::firstExistingFile(chain).string(), (system / "DefUser.ini").string());

	File::write_all_text((system / "SE-User.ini").string(), "x");
	CHECK_EQ(UserDirs::firstExistingFile(chain).string(), (system / "SE-User.ini").string());

	// The per-user copy beats everything in the install
	File::write_all_text((config / "SE-User.ini").string(), "x");
	CHECK_EQ(UserDirs::firstExistingFile(chain).string(), (config / "SE-User.ini").string());
}

#if !defined(WIN32) && !defined(__APPLE__)

TEST(UserDirsFirstExistingFileFindsALowercaseUserIniBeforeDefUserIni)
{
	// An old install on a case-sensitive file system: only user.ini and the stock DefUser.ini exist
	Sandbox box("lowercase");
	const std::filesystem::path system = box.Game() / "System";
	File::write_all_text((system / "user.ini").string(), "x");
	File::write_all_text((system / "DefUser.ini").string(), "x");

	CHECK_EQ(UserDirs::firstExistingFile(UserDirs::userIniChain(box.Out(), system)).string(), (system / "user.ini").string());

	// User.ini, when present, still comes first
	File::write_all_text((system / "User.ini").string(), "x");
	CHECK_EQ(UserDirs::firstExistingFile(UserDirs::userIniChain(box.Out(), system)).string(), (system / "User.ini").string());
}

#endif
