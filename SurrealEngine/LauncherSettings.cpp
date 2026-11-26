
#include "Precomp.h"
#include "LauncherSettings.h"
#include "Utils/JsonValue.h"
#include "Utils/File.h"
#include "Utils/UserDirs.h"

// The member's default is written out as a number so that the settings struct reads on its own; this keeps it equal to
// the default a settings file without the key gets
static_assert(FramePacer::DefaultLimit == 60, "LauncherSettings::RenderDevice::FrameRateLimit must default to FramePacer::DefaultLimit");

int ReadFrameRateLimit(const JsonValue& renderDevice)
{
	// A settings file written before the key existed must get the default, and a missing number reads as 0, which is off
	const JsonValue& value = renderDevice["FrameRateLimit"];
	return value.is_number() ? value.to_int() : FramePacer::DefaultLimit;
}

void WriteFrameRateLimit(JsonValue& renderDevice, int limit)
{
	renderDevice["FrameRateLimit"] = JsonValue::number(limit);
}

int FrameLimitFromFieldText(const std::string& text, int previous)
{
	// Zero means off, so a typo must not be read as a number. The previous value is clamped too, so that a hand-edited
	// file that holds a value out of range is never written back as it is
	std::optional<int> value = FramePacer::ParseDigits(text);
	return FramePacer::ClampLimit(value ? *value : previous);
}

LauncherSettings& LauncherSettings::Get()
{
	static LauncherSettings settings;
	return settings;
}

static std::string GetSettingsFilename()
{
	return (UserDirs::appConfigDir() / "Settings.json").string();
}

LauncherSettings::LauncherSettings()
{
	try
	{
		JsonValue settings = JsonValue::parse(File::read_all_text(GetSettingsFilename()));

		std::string rendevtype = settings["RenderDevice"]["Type"].to_string();
		if (rendevtype == "Vulkan")
			RenderDevice.Type = RenderDeviceType::Vulkan;
		else if (rendevtype == "D3D11")
			RenderDevice.Type = RenderDeviceType::D3D11;
		else if (rendevtype == "D3D12")
			RenderDevice.Type = RenderDeviceType::D3D12;
		else if (rendevtype == "OpenGL")
			RenderDevice.Type = RenderDeviceType::OpenGL;

		std::string rendevaa = settings["RenderDevice"]["Antialias"].to_string();
		if (rendevaa == "Off")
			RenderDevice.Antialias = AntialiasMode::Off;
		else if (rendevaa == "MSAA2x")
			RenderDevice.Antialias = AntialiasMode::MSAA2x;
		else if (rendevaa == "MSAA4x")
			RenderDevice.Antialias = AntialiasMode::MSAA4x;

		std::string rendevlight = settings["RenderDevice"]["Light"].to_string();
		if (rendevlight == "Normal")
			RenderDevice.Light = LightMode::Normal;
		else if (rendevlight == "OneX")
			RenderDevice.Light = LightMode::OneX;
		else if (rendevlight == "BrighterActors")
			RenderDevice.Light = LightMode::BrighterActors;

		std::string rendevgamma = settings["RenderDevice"]["Gamma"].to_string();
		if (rendevgamma == "D3D9")
			RenderDevice.Gamma = GammaMode::D3D9;
		else if (rendevgamma == "XOpenGL")
			RenderDevice.Gamma = GammaMode::XOpenGL;

		RenderDevice.GammaCorrectScreenshots = settings["RenderDevice"]["GammaCorrectScreenshots"].to_boolean();
		RenderDevice.UseVSync = settings["RenderDevice"]["UseVSync"].to_boolean();
		RenderDevice.FrameRateLimit = ReadFrameRateLimit(settings["RenderDevice"]);
		RenderDevice.Hdr = settings["RenderDevice"]["Hdr"].to_boolean();
		RenderDevice.HdrScale = settings["RenderDevice"]["HdrScale"].to_int();
		RenderDevice.Bloom = settings["RenderDevice"]["Bloom"].to_boolean();
		RenderDevice.BloomAmount = settings["RenderDevice"]["BloomAmount"].to_int();
		RenderDevice.UseDebugLayer = settings["RenderDevice"]["UseDebugLayer"].to_boolean();

		for (const JsonValue& jsonItem : settings["Games"]["SearchList"].items())
		{
			Games.SearchList.push_back(jsonItem.to_string());
		}
		Games.LastSelected = settings["Games"]["LastSelected"].to_int();
	}
	catch (...)
	{
	}
}

void LauncherSettings::Save()
{
	JsonValue rendev = JsonValue::object();

	switch (RenderDevice.Type)
	{
	default:
	case RenderDeviceType::Vulkan: rendev["Type"] = JsonValue::string("Vulkan"); break;
	case RenderDeviceType::D3D11: rendev["Type"] = JsonValue::string("D3D11"); break;
	case RenderDeviceType::D3D12: rendev["Type"] = JsonValue::string("D3D12"); break;
	case RenderDeviceType::OpenGL: rendev["Type"] = JsonValue::string("OpenGL"); break;
	}

	switch (RenderDevice.Antialias)
	{
	default:
	case AntialiasMode::Off: rendev["Antialias"] = JsonValue::string("Off"); break;
	case AntialiasMode::MSAA2x: rendev["Antialias"] = JsonValue::string("MSAA2x"); break;
	case AntialiasMode::MSAA4x: rendev["Antialias"] = JsonValue::string("MSAA4x"); break;
	}

	switch (RenderDevice.Light)
	{
	default:
	case LightMode::Normal: rendev["Light"] = JsonValue::string("Normal"); break;
	case LightMode::OneX: rendev["Light"] = JsonValue::string("OneX"); break;
	case LightMode::BrighterActors: rendev["Light"] = JsonValue::string("BrighterActors"); break;
	}

	switch (RenderDevice.Gamma)
	{
	default:
	case GammaMode::D3D9: rendev["Gamma"] = JsonValue::string("D3D9"); break;
	case GammaMode::XOpenGL: rendev["Gamma"] = JsonValue::string("XOpenGL"); break;
	}

	rendev["GammaCorrectScreenshots"] = JsonValue::boolean(RenderDevice.GammaCorrectScreenshots);
	rendev["UseVSync"] = JsonValue::boolean(RenderDevice.UseVSync);
	WriteFrameRateLimit(rendev, RenderDevice.FrameRateLimit);
	rendev["Hdr"] = JsonValue::boolean(RenderDevice.Hdr);
	rendev["HdrScale"] = JsonValue::number(RenderDevice.HdrScale);
	rendev["Bloom"] = JsonValue::boolean(RenderDevice.Bloom);
	rendev["BloomAmount"] = JsonValue::number(RenderDevice.BloomAmount);
	rendev["UseDebugLayer"] = JsonValue::boolean(RenderDevice.UseDebugLayer);

	JsonValue games = JsonValue::object();
	games["SearchList"] = JsonValue::array(Games.SearchList);
	games["LastSelected"] = JsonValue::number(Games.LastSelected);

	JsonValue settings = JsonValue::object();
	settings["RenderDevice"] = std::move(rendev);
	settings["Games"] = std::move(games);

	const std::string filename = GetSettingsFilename();
	Directory::create(fs::path(filename).parent_path().string());
	File::write_all_text(filename, settings.to_json(true));
}
