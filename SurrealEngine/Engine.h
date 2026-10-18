#pragma once

#include "Utils/Logger.h"
#include "Math/vec.h"
#include "Math/mat.h"
#include "Math/floating.h"
#include "RenderDevice/RenderDevice.h"
#include "GameWindow.h"
#include "Packages/Core/Properties/UProperty.h"
#include "Packages/Engine/Actors/UActor.h"
#include "UnrealURL.h"
#include "GameFolder.h"
#include <set>
#include <list>

class RenderSubsystem;
class PackageManager;
class UObject;
class ULevel;
class UModel;
class GameWindow;
class UTexture;
class UActor;
class UFont;
class UMesh;
class ULodMesh;
class USkeletalMesh;
class ULevelSummary;
class ULevelInfo;
class UZoneInfo;
class UClient;
class UViewport;
class UCanvas;
class UConsole;
class UPlayerPawn;
class UGameInfo;
class UGameReplicationInfo;
class UPlayerReplicationInfo;
class UGameEngine;
class USurrealRenderDevice;
class USurrealAudioDevice;
class USurrealNetworkDevice;
class USurrealClient;
class BspSurface;
class BspNode;
class LightMapIndex;
class FrustumPlanes;
class AudioSubsystem;
class Rotator;
class ExpressionValue;
class UnrealURL;
class VideoPlayer;
class UnrealMipmap;
class UFloatProperty;
class UConversationMissionList;
class UConversationList;
class UDXSaveInfo;
class UDeusExLevelInfo;
class URootWindow;
class UGC;
struct TextureInfo;
struct SceneNode;
struct SurfaceFacet;
struct MeshFace;

static constexpr int32_t DONT_SAVE_GAME = -2; // Since -1 might be used as the autosave slot in some UE1 games...

// The buttons and axes the harness holds down for a fixed time (the "hold" console command). The
// clock is the --exec one, seconds since the first login. Plain values only, so the bookkeeping
// can be tested without an engine.
class InputHoldSchedule
{
public:
	enum class Kind { Button, Axis };

	struct Hold
	{
		Kind kind = Kind::Button;
		std::string name;	// The player property: a byte or bool for a button, a float for an axis
		float value = 0.0f;	// The axis value while held; unused for a button
		double until = 0.0;	// The clock time at which the hold ends
	};

	// A hold of the same kind and name replaces the one already there, so the latest command wins
	void Add(Kind kind, const std::string& name, float value, double until);

	// Removes and returns the holds that have ended by now
	Array<Hold> TakeExpired(double now);

	// Removes and returns every hold
	Array<Hold> TakeAll();

	const Array<Hold>& Active() const { return holds; }

private:
	Array<Hold> holds;
};

struct HoldParseResult
{
	InputHoldSchedule::Hold hold;	// Valid only when the command was accepted
	std::string error;				// Names the offending part; empty when the command was accepted

	bool ok() const { return error.empty(); }
};

// Parses "hold button <name> <seconds>" and "hold axis <name> <speed> <seconds>" (the arguments
// as the console splits them) into a hold that ends <seconds> after now. The duration must be
// positive and finite, the speed finite.
HoldParseResult ParseHoldCommand(const Array<std::string>& args, double now);

// Why a player property cannot be held as this kind of input, or an empty string when it can.
// Only properties the scripts declare as input are held: the engine writes a held property every
// frame, so any other byte, bool or float of the player (GroundSpeed, Physics) would be left at
// the held value for good.
std::string HoldPropertyRefusal(InputHoldSchedule::Kind kind, const std::string& name, ExpressionValueType type, PropertyFlags flags);

class Engine : public GameWindowHost
{
public:
	Engine(GameLaunchInfo launchinfo);
	~Engine();

	void Run();

	// The --exec commands that never became due; read once Run has returned.
	Array<ExecItem> UnrunExecCommands() const;
	void ClientTravel(const std::string& URL, ETravelType travelType, bool transferItems);
	UnrealURL GetDefaultURL(const std::string& map);
	void LoadEntryMap();
	void LoadMap(const UnrealURL& url, const std::map<std::string, std::string>& travelInfo = {});
	void LoadFromSaveFile(const UnrealURL& url);
	void SaveGameToSlot(int32_t slotNum, const std::string& saveDescription) const;
	void UnloadMap();
	void LoginPlayer();
	void PossessSavedPlayer();

	UObject* FindObject(NameString name, NameString className);

	std::string ConsoleCommand(UObject* context, const std::string& command, BitfieldBool& found);

	void UpdateInput(float timeElapsed);
	void InputCommand(const std::string& command, EInputKey key, int delta);

	void LockCursor();
	void UnlockCursor();

	bool ExecCommand(const Array<std::string>& args);
	Array<std::string> GetArgs(const std::string& commandline);
	Array<std::string> GetSubcommands(const std::string& commandline);

	void PlayAVI(const Array<std::string>& args);
	UnrealMipmap* PlayVideo(VideoPlayer* video, UnrealMipmap* background);

	UConversationList* GetDeusExMission();

	void UpdateAudio();

	void OpenWindow();
	void CloseWindow();
	void TickWindow();

	void Key(std::string key);
	void InputEvent(EInputKey key, EInputType type, int delta = 0);

	void OnWindowPaint() override;
	void OnWindowMouseMove(const Point& pos) override;
	void OnWindowMouseDown(const Point& pos, EInputKey key) override;
	void OnWindowMouseDoubleclick(const Point& pos, EInputKey key) override;
	void OnWindowMouseUp(const Point& pos, EInputKey key) override;
	void OnWindowMouseWheel(const Point& pos, EInputKey key) override;
	void OnWindowRawMouseMove(int dx, int dy) override;
	void OnWindowKeyChar(std::string chars) override;
	void OnWindowKeyDown(EInputKey key) override;
	void OnWindowKeyUp(EInputKey key) override;
	void OnWindowGeometryChanged() override;
	void OnWindowClose() override;
	void OnWindowActivated() override;
	void OnWindowDeactivated() override;
	void OnWindowDpiScaleChanged() override;

	void SetPause(bool value);

	UZoneInfo* GetZoneActor(int zoneIndex);

	std::string ParseClassName(std::string className);

	UGameEngine* gameengine = nullptr;
	USurrealRenderDevice* renderdev = nullptr;
	USurrealAudioDevice* audiodev = nullptr;
	USurrealNetworkDevice* netdev = nullptr;
	USurrealClient* client = nullptr;
	UViewport* viewport = nullptr;
	UCanvas* canvas = nullptr;
	UGC* dxgc = nullptr;
	UDXSaveInfo* dxSaveInfo = nullptr;
	UConversationMissionList* dxConMissionList = nullptr;
	UConsole* console = nullptr;
	URootWindow* dxRootWindow = nullptr;

	UFloatProperty* floatprop = nullptr;

	double TotalTime = 0.0;

	Package* EntryLevelPackage = nullptr;
	ULevel* EntryLevel = nullptr;
	ULevelInfo* EntryLevelInfo = nullptr;
	UGameInfo* EntryGameInfo = nullptr;

	Package* LevelPackage = nullptr;
	ULevel* Level = nullptr;
	ULevelInfo* LevelInfo = nullptr;
	UGameInfo* GameInfo = nullptr;
	UDeusExLevelInfo* DeusExLevelInfo = nullptr;

	UTexture* DefaultTexture = nullptr;

	Package* deusExPackage = nullptr;
	struct
	{
		UnrealURL URL;
		ETravelType TravelType = ETravelType::TRAVEL_Absolute;
		bool TransferItems = false;
		bool TravelToEntryLevel = false;
	} ClientTravelInfo;

	struct
	{
		int32_t SaveGameSlot = DONT_SAVE_GAME;
		std::string SaveGameDescription;
	} SaveGameInfo;

	GameLaunchInfo LaunchInfo;
	std::unique_ptr<PackageManager> packages;
	std::unique_ptr<GameWindow> window; // TODO: Move into UViewport
	std::unique_ptr<RenderSubsystem> render;

	int MouseMoveX = 0;
	int MouseMoveY = 0;

	float CalcTimeElapsed();

	UActor* CameraActor = nullptr;
	vec3 CameraLocation = vec3(0.0f);
	Rotator CameraRotation = Rotator(0,0,0);
	float CameraFovAngle = 95.0f;

	std::string windowingSystemName;

	// Collision debug
	BspNode* PlayerBspNode = nullptr;
	vec3 PlayerHitNormal = vec3(0.0f);
	vec3 PlayerHitLocation = vec3(0.0f);

	bool quit = false;

	// --exec: LaunchInfo.execSchedule[execCursor...] has not run yet. The clock starts at the
	// first login and later logins (map changes) leave it alone; negative until then.
	void NotePlayerLogin();
	void RunDueExecCommands();
	double execLoginSeconds = -1.0;
	size_t execCursor = 0;

	// The "hold" console command keeps a button or an axis down for a fixed time, on the --exec
	// clock. A hold acts on the player below the key bindings, so it neither needs nor disturbs
	// activeInputButtons and activeInputAxes; UpdateInput applies and ends it.
	InputHoldSchedule inputHolds;
	double LoginClockSeconds() const;
	bool HoldInput(const Array<std::string>& args);
	void ReleaseHolds(const Array<InputHoldSchedule::Hold>& holds);

	// Writes a STATE line to the smoke log when the player's state name changes, and on the first
	// frame after every LOGIN, with the player's location. Called once per frame, so a state entered
	// and left between two frames is not seen.
	void NoteStateChanges();
	UPlayerPawn* stateSeenActor = nullptr;
	NameString stateSeenName;

	uint64_t lastTime = 0;

	void LoadEngineSettings();

	void LoadKeybindings();
	std::map<std::string, std::string> keybindings;
	std::map<std::string, std::string> inputAliases;
	static const char* keynames[256];

	struct ActiveInputAxis
	{
		float Value;
		EInputKey Key;
	};

	std::map<std::string, EInputKey> activeInputButtons;
	std::map<std::string, ActiveInputAxis> activeInputAxes;

	std::function<void()> tickDebugger;

	bool getEditorMode() const { return m_EditorMode; }
	void setEditorMode(const bool value) { m_EditorMode = value; }

	bool getDXWindowDebugMode() const { return m_DrawDebugDXWindowHierarchy; }

private:
	std::map<std::string, std::string> CreateTravelInfo(bool transferItems);

	void LogGamePackageSHA1Sums() const;
	void GetLevelInfoObject();
	void GetLevelObject();
	void LinkActorsToLevel();

	bool m_EditorMode = false; // Set this to true to allow rendering of invisible polys.
	bool m_GamePaused = false;

	bool m_DrawDebugDXWindowHierarchy = false; // If set to true, engine will show the Deus Ex window hierarchy

	bool khgSplashScreen = false;
	bool playingAvi = false;
	bool skipAvi = false;
};

extern Engine* engine;
