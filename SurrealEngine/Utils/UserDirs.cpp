#include "Precomp.h"
#include "Utils/UserDirs.h"
#include "GameFolder.h"
#include "Utils/Logger.h"
#include <algorithm>
#include <mutex>
#include <optional>
#include <stdexcept>
#ifdef WIN32
#include <Windows.h>
#include <Shlobj.h>
#include "Utils/UTF16.h"
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace
{
	// Names the engine's own directory beneath the config and data roots
	const char* const AppDirName = "SurrealEngine";

	constexpr size_t MaxKeyLeafLength = 32;

	bool IsSeparator(char c)
	{
#ifdef WIN32
		return c == '/' || c == '\\';
#else
		return c == '/';
#endif
	}

	// path::parent_path() and filename() treat "a/b/" as "a/b/" plus an empty name, which would let a
	// trailing separator leak into results and into component-wise comparisons.
	fs::path StripTrailingSeparators(const fs::path& path)
	{
		std::string text = path.string();
		while (text.size() > 1 && IsSeparator(text.back()))
			text.pop_back();
		return fs::path(text);
	}

	std::string CurrentUserId()
	{
#ifdef WIN32
		// Only reached when LocalAppData cannot be resolved; there is no uid to name the directory by
		return "user";
#else
		return std::to_string(getuid());
#endif
	}

	// Last resort for a process with no usable home: a directory of its own per user, so two users on one
	// machine never share (or fight over) it. Not an error: the engine must start with a bare environment.
	fs::path TempFallbackRoot()
	{
		std::error_code ec;
		fs::path temp = fs::temp_directory_path(ec);
		if (ec)
			temp = "/tmp";

		const fs::path root = temp / (std::string(AppDirName) + "-" + CurrentUserId());

		static std::once_flag warned;
		std::call_once(warned, [&]() { LogMessage("Warning: no usable home directory in the environment; per-user files go under " + root.string()); });
		return root;
	}

#ifdef WIN32
	std::optional<fs::path> LocalAppData()
	{
		PWSTR path = nullptr;
		if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &path)))
			return std::nullopt;
		try
		{
			std::string folder = from_utf16(path);
			CoTaskMemFree(path);
			return StripTrailingSeparators(folder);
		}
		catch (...)
		{
			CoTaskMemFree(path);
			return std::nullopt;
		}
	}
#else
	bool IsUsable(const char* value)
	{
		return value && *value && fs::path(value).is_absolute();
	}

	fs::path PosixRoot(const UserDirs::EnvLookup& env, const char* xdgName, const char* homeSubdirectory)
	{
		if (const char* xdg = env(xdgName); IsUsable(xdg))
			return StripTrailingSeparators(xdg);

		if (const char* home = env("HOME"); IsUsable(home))
			return StripTrailingSeparators(home) / homeSubdirectory;

		return TempFallbackRoot();
	}
#endif

	// The leaf of the install folder as it goes into the key: only [A-Za-z0-9._-], each other character
	// (a UTF-8 sequence counts once) becoming "_", cut to MaxKeyLeafLength, "root" when nothing is left.
	std::string KeyLeaf(const std::string& rootFolder)
	{
		const fs::path normal = StripTrailingSeparators(fs::path(rootFolder).lexically_normal());
		const std::string leaf = normal.filename().string();

		std::string key;
		for (unsigned char c : leaf)
		{
			if ((c & 0xC0) == 0x80) // Continuation byte of a character that was already replaced
				continue;

			const bool allowed = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
			key += allowed ? static_cast<char>(c) : '_';
		}

		if (key.size() > MaxKeyLeafLength)
			key.resize(MaxKeyLeafLength);
		if (key.empty())
			key = "root";
		return key;
	}

	// Symbolic links and ".." are resolved, so a path cannot reach the install folder by a detour. The
	// parts that do not exist yet (the usual case for a file about to be created) are normalized
	// lexically. A relative path is made absolute first because weakly_canonical leaves a relative
	// path relative when none of its leading elements exist.
	fs::path Canonical(const fs::path& path)
	{
		std::error_code ec;
		const fs::path absolute = fs::absolute(path, ec);
		if (ec)
			return StripTrailingSeparators(path.lexically_normal());

		fs::path resolved = fs::weakly_canonical(absolute, ec);
		if (ec)
			resolved = absolute.lexically_normal();
		return StripTrailingSeparators(resolved);
	}

	// root is already canonical; path is resolved here
	void RefuseIfInside(const fs::path& root, const fs::path& path)
	{
		const fs::path target = Canonical(path);
		if (std::mismatch(root.begin(), root.end(), target.begin(), target.end()).first == root.end())
			throw std::runtime_error("refusing to write inside the game folder: " + path.string());
	}

	std::mutex ForbiddenRootMutex;
	std::optional<fs::path> ForbiddenRoot;
}

UserDirs::EnvLookup UserDirs::processEnv()
{
	return [](const char* name) { return std::getenv(name); };
}

fs::path UserDirs::configRoot(const EnvLookup& env)
{
#ifdef WIN32
	if (std::optional<fs::path> appData = LocalAppData())
		return *appData;
	return TempFallbackRoot();
#else
	return PosixRoot(env, "XDG_CONFIG_HOME", ".config");
#endif
}

fs::path UserDirs::dataRoot(const EnvLookup& env)
{
#ifdef WIN32
	if (std::optional<fs::path> appData = LocalAppData())
		return *appData;
	return TempFallbackRoot();
#else
	return PosixRoot(env, "XDG_DATA_HOME", ".local/share");
#endif
}

fs::path UserDirs::appConfigDir()
{
	return configRoot(processEnv()) / AppDirName;
}

std::string UserDirs::gameKey(const GameLaunchInfo& info)
{
	return info.gameExecutableName + "-" + info.gameVersionString + "-" + KeyLeaf(info.gameRootFolder);
}

UserDirs::GameDirs UserDirs::gameDirs(const GameLaunchInfo& info, const EnvLookup& env)
{
	// "." and "folder" must name the same install as its full path does
	GameLaunchInfo keyed = info;
	if (!keyed.gameRootFolder.empty() && fs::path(keyed.gameRootFolder).is_relative())
	{
		std::error_code ec;
		const fs::path absolute = fs::absolute(keyed.gameRootFolder, ec);
		if (!ec)
			keyed.gameRootFolder = absolute.lexically_normal().string();
	}

	const std::string key = gameKey(keyed);

	// One user directory may serve several games, so each keeps to its own subdirectory of it
	if (!info.userDir.empty())
		return { fs::path(info.userDir) / key, fs::path(info.userDir) / key };

	return { configRoot(env) / AppDirName / key, dataRoot(env) / AppDirName / key };
}

fs::path UserDirs::resolveUserDirArg(const std::string& arg)
{
	if (arg.empty())
		throw std::invalid_argument("--user-dir needs a directory path");

	return Canonical(fs::path(arg));
}

std::vector<fs::path> UserDirs::userIniChain(const fs::path& configDir, const fs::path& systemDir)
{
	return { configDir / "SE-User.ini", systemDir / "SE-User.ini", systemDir / "User.ini", systemDir / "user.ini", systemDir / "DefUser.ini" };
}

fs::path UserDirs::firstExistingFile(const std::vector<fs::path>& candidates)
{
	for (const fs::path& candidate : candidates)
	{
		std::error_code ec;
		if (fs::is_regular_file(candidate, ec))
			return candidate;
	}
	return candidates.empty() ? fs::path() : candidates.back();
}

void WritePolicy::setForbiddenRoot(const fs::path& gameRootFolder)
{
	// An empty root would switch the guard off without anyone having asked for that
	if (gameRootFolder.empty())
		throw std::invalid_argument("WritePolicy: the game folder to protect is empty");

	const fs::path root = Canonical(gameRootFolder);
	std::lock_guard<std::mutex> lock(ForbiddenRootMutex);
	ForbiddenRoot = root;
}

void WritePolicy::clearForbiddenRoot()
{
	std::lock_guard<std::mutex> lock(ForbiddenRootMutex);
	ForbiddenRoot.reset();
}

void WritePolicy::checkAgainst(const fs::path& folder, const fs::path& path)
{
	// Nothing to compare, and an empty name cannot be created anywhere
	if (folder.empty() || path.empty())
		return;

	RefuseIfInside(Canonical(folder), path);
}

void WritePolicy::check(const fs::path& path)
{
	fs::path root;
	{
		std::lock_guard<std::mutex> lock(ForbiddenRootMutex);
		if (!ForbiddenRoot)
			return;
		root = *ForbiddenRoot;
	}

	// An empty name cannot be created anywhere, so there is nothing to protect
	if (path.empty())
		return;

	RefuseIfInside(root, path);
}
