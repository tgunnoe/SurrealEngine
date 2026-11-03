#include "Precomp.h"
#include "TestRunner.h"
#include "Package/IniFile.h"
#include "Utils/File.h"
#include <filesystem>

// IniFile::UpdateIfExists is how the engine writes its per-user ini files while reading the install's
// copy. These pin that a write goes to the named target and to nothing else.

TEST(UserDirsIniWriteLeavesTheLoadedFileAlone)
{
	TempFile loaded("userdirs-loaded.ini");
	TempFile target("userdirs-target.ini");
	File::write_all_text(loaded.Path, "[Engine.Engine]\nLanguage=int\nGameRenderDevice=Old\n");
	File::try_delete(target.Path);

	IniFile ini(loaded.Path);
	ini.SetValue("Engine.Engine", "GameRenderDevice", "New");
	ini.UpdateIfExists(target.Path);

	CHECK_EQ(File::read_all_text(loaded.Path), std::string("[Engine.Engine]\nLanguage=int\nGameRenderDevice=Old\n"));
	IniFile written(target.Path);
	CHECK_EQ(written.GetValue("Engine.Engine", "GameRenderDevice"), std::string("New"));
	CHECK_EQ(written.GetValue("Engine.Engine", "Language"), std::string("int"));
}

TEST(UserDirsIniWriteCreatesTheTargetEvenWhenNothingChanged)
{
	TempFile loaded("userdirs-unchanged.ini");
	TempFile target("userdirs-unchanged-target.ini");
	File::write_all_text(loaded.Path, "[Engine.Engine]\nLanguage=int\n");
	File::try_delete(target.Path);

	IniFile ini(loaded.Path);
	ini.UpdateIfExists(target.Path);

	CHECK(std::filesystem::exists(target.Path));
	CHECK_EQ(IniFile(target.Path).GetValue("Engine.Engine", "Language"), std::string("int"));
}
