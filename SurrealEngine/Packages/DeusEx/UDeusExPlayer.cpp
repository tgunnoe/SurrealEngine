
#include "Precomp.h"
#include "UDeusExPlayer.h"
#include "Engine.h"
#include "Package/PackageManager.h"
#include "Packages/DeusEx/UGameDirectory.h"
#include "Packages/Extension/Flags/UFlagBase.h"
#include "Utils/Logger.h"

void UDeusExPlayer::ConBindEvents()
{
	DeusExConBindEvents();
}

namespace
{
	// What the game keeps for the player (history, log, image notes) is created in the player's package with
	// the player as its outer, so a save that exports the player exports it too
	UObject* CreatePlayerObject(UDeusExPlayer* player, const char* className, const char* name)
	{
		UClass* cls = engine->packages->FindClass(className);
		UObject* obj = player->package->NewObject(name, cls, ObjectFlags::NoFlags);
		obj->Outer() = player;
		return obj;
	}
}

UObject* UDeusExPlayer::CreateDataVaultImageNoteObject()
{
	return CreatePlayerObject(this, "DeusEx.DataVaultImageNote", "DataVaultImageNote");
}

UObject* UDeusExPlayer::CreateDumpLocationObject()
{
	return CreatePlayerObject(this, "DeusEx.DumpLocation", "DumpLocation");
}

UObject* UDeusExPlayer::CreateGameDirectoryObject()
{
	// The menus' directory has native members and is never saved, so it stays in the transient package
	if (!m_GameDirectory)
	{
		auto cls = engine->packages->FindClass("DeusEx.GameDirectory");
		m_GameDirectory = Cast<UDXGameDirectory>(engine->packages->GetTransientPackage()->NewObject("GameDirectory", cls, ObjectFlags::Transient));
	}

	return m_GameDirectory;
}

UObject* UDeusExPlayer::CreateHistoryEvent()
{
	return CreatePlayerObject(this, "ConSys.ConHistoryEvent", "ConHistoryEvent");
}

UObject* UDeusExPlayer::CreateHistoryObject()
{
	return CreatePlayerObject(this, "ConSys.ConHistory", "ConHistory");
}

UObject* UDeusExPlayer::CreateLogObject()
{
	return CreatePlayerObject(this, "DeusEx.DeusExLog", "DeusExLog");
}

void UDeusExPlayer::DeleteSaveGameFiles(std::optional<std::string> saveDirectory)
{
	LogUnimplemented("DeusExPlayer.DeleteSaveGameFiles");
}

std::string UDeusExPlayer::GetDeusExVersion()
{
	return "1.112fm. Surreal Engine Edition!";
}

void UDeusExPlayer::SaveGame(int saveIndex, std::optional<std::string> saveDesc)
{
	engine->SaveGameInfo.SaveGameSlot = saveIndex;
	engine->SaveGameInfo.SaveGameDescription = saveDesc.value_or("");
}

NameString UDeusExPlayer::SetBoolFlagFromString(const std::string& flagNameString, bool bValue)
{
	NameString flagName(flagNameString);
	if (UFlagBase* flagBase = FlagBase())
		flagBase->SetBool(flagName, bValue, {}, {});
	return flagName;
}

void UDeusExPlayer::UnloadTexture(UObject* Texture)
{
	// Nothing going on here because SE never unloads textures atm. This is just here so it doesn't throw LogUnimplemented.
}
