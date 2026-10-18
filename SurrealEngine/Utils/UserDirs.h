#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

struct GameLaunchInfo;

// Where the engine keeps everything it writes. A game's install folder is never one of those places.
namespace UserDirs
{
	// Looks up an environment variable and returns null when it is not set. Callers pass
	// processEnv() in production; tests pass their own table so the result never depends on the host.
	using EnvLookup = std::function<const char*(const char*)>;

	EnvLookup processEnv();

	// XDG_CONFIG_HOME when it is set to an absolute path, else $HOME/.config. When HOME cannot be used
	// either, a per-user directory under the system temp folder (with one warning in the engine log),
	// never an exception. Windows: the LocalAppData folder. Neither result ends in a separator.
	std::filesystem::path configRoot(const EnvLookup& env);

	// XDG_DATA_HOME when it is set to an absolute path, else $HOME/.local/share, with the same fallbacks.
	std::filesystem::path dataRoot(const EnvLookup& env);

	// <configRoot>/SurrealEngine, resolved from the process environment: the launcher settings, the crash
	// reports and the last-run log, which belong to the engine rather than to one game.
	std::filesystem::path appConfigDir();

	// "<gameExecutableName>-<gameVersionString>-<leaf>", where leaf is the last component of the install
	// folder restricted to [A-Za-z0-9._-] and at most 32 characters ("root" when there is none). It depends
	// on those three fields only, so the same install always lands in the same directory.
	std::string gameKey(const GameLaunchInfo& info);

	struct GameDirs
	{
		std::filesystem::path config; // The SE- ini files
		std::filesystem::path data;   // Save/ and Export/
	};

	// <configRoot>/SurrealEngine/<key> and <dataRoot>/SurrealEngine/<key>, or info.userDir for both when
	// the run was given one. A relative install folder is taken relative to the current directory.
	GameDirs gameDirs(const GameLaunchInfo& info, const EnvLookup& env);

	// Turns the value of --user-dir into an absolute path (relative to the current directory).
	std::filesystem::path resolveUserDirArg(const std::string& arg);

	// Where a game's user ini may be read from, most specific first: the per-user SE-User.ini, the install's
	// SE-User.ini (written by earlier builds), User.ini, user.ini (for case-sensitive file systems) and DefUser.ini.
	std::vector<std::filesystem::path> userIniChain(const std::filesystem::path& configDir, const std::filesystem::path& systemDir);

	// The first candidate that is a regular file, else the last one, so that failing to open it names a file
	// someone would look for.
	std::filesystem::path firstExistingFile(const std::vector<std::filesystem::path>& candidates);
}

// Refuses writes inside the game's install folder. File::create_always, File::rename, File::try_delete,
// Directory::create and Directory::remove_all call check(), so no engine code needs to remember to.
class WritePolicy
{
public:
	// Called once the game is selected; from then on check() refuses the folder and everything below it.
	static void setForbiddenRoot(const std::filesystem::path& gameRootFolder);

	// Lifts the refusal. Production code never calls this; tests do, to leave no state behind.
	static void clearForbiddenRoot();

	// Throws std::runtime_error when path is the forbidden root or lies below it (symbolic links and ".."
	// are resolved first). Before a root is set, everything is allowed: the smoke log opens earlier.
	static void check(const std::filesystem::path& path);

	// The same refusal against a folder given by the caller instead of the stored root, which it leaves
	// alone. For output that is opened before a game is selected, such as a log named on the command line.
	static void checkAgainst(const std::filesystem::path& folder, const std::filesystem::path& path);
};
