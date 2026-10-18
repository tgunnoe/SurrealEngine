#include "Precomp.h"
#include "Utils/UserDirs.h"
#include "GameFolder.h"

namespace fs = std::filesystem;

UserDirs::EnvLookup UserDirs::processEnv()
{
	return [](const char* name) { return std::getenv(name); };
}

fs::path UserDirs::configRoot(const EnvLookup& env)
{
	return {};
}

fs::path UserDirs::dataRoot(const EnvLookup& env)
{
	return {};
}

fs::path UserDirs::appConfigDir()
{
	return {};
}

std::string UserDirs::gameKey(const GameLaunchInfo& info)
{
	return {};
}

UserDirs::GameDirs UserDirs::gameDirs(const GameLaunchInfo& info, const EnvLookup& env)
{
	return {};
}

fs::path UserDirs::resolveUserDirArg(const std::string& arg)
{
	return {};
}

void WritePolicy::setForbiddenRoot(const fs::path& gameRootFolder)
{
}

void WritePolicy::clearForbiddenRoot()
{
}

void WritePolicy::check(const fs::path& path)
{
}
