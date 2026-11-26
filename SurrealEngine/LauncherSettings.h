#pragma once

#include <string>
#include "Utils/FramePacer.h"

class JsonValue;

enum class RenderDeviceType
{
	Vulkan,
	D3D11,
	D3D12,
	OpenGL
};

enum class AntialiasMode
{
	Off,
	MSAA2x,
	MSAA4x
};

enum class LightMode
{
	Normal,
	OneX,
	BrighterActors
};

enum class GammaMode
{
	D3D9,
	XOpenGL
};

// The frame rate limit as the settings file holds it: the RenderDevice object in, the limit out. A key that is
// missing or not a number gives the default, never 0 (which means off).
int ReadFrameRateLimit(const JsonValue& renderDevice);
void WriteFrameRateLimit(JsonValue& renderDevice, int limit);

// What the launcher's frame rate field saves: the clamped number in the text, or the previous value when the
// text is not a plain whole number, so a typo never turns the limit off.
int FrameLimitFromFieldText(const std::string& text, int previous);

class LauncherSettings
{
public:
	static LauncherSettings& Get();
	void Save();

	struct
	{
		RenderDeviceType Type = RenderDeviceType::Vulkan;
		bool UseVSync = true;
		int FrameRateLimit = FramePacer::DefaultLimit; // Frames per second; 0 means off, otherwise 20 to 1000 once clamped
		AntialiasMode Antialias = AntialiasMode::MSAA4x;
		LightMode Light = LightMode::Normal;
		GammaMode Gamma = GammaMode::D3D9;
		bool GammaCorrectScreenshots = false;
		bool Hdr = false;
		int HdrScale = 128;
		bool Bloom = false;
		int BloomAmount = 128;
		bool UseDebugLayer = false;
	} RenderDevice;

	struct
	{
		Array<std::string> SearchList;
		int LastSelected = -1;
	} Games;

private:
	LauncherSettings();
};
