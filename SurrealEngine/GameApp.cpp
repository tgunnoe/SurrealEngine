
#include "Precomp.h"
#include "Utils/Exception.h"
#include "Utils/Logger.h"
#include "Utils/CommandLine.h"
#include "GameApp.h"
#include "GameFolder.h"
#include "Engine.h"
#include "UI/WidgetResourceData.h"
#include "UI/ErrorWindow/ErrorWindow.h"
#include "UI/Launcher/LauncherWindow.h"
#include "Utils/File.h"
#include <stdexcept>
#include <iostream>

int GameApp::main(Array<std::string> args)
{
	SmokeLog::Get(); // Starts the --exit-after clock

	InitWidgetResources("dark");

	CommandLine cmd(args);
	commandline = &cmd;

	const bool autostart = commandline->HasArg("", "--autostart");
	if (commandline->HasArg("", "--smoke-log"))
	{
		const std::string path = commandline->GetArg("", "--smoke-log");
		if (!SmokeLog::Get()->Open(path))
		{
			std::cerr << "Unable to open smoke log " << path << "\n";
			return 1;
		}
		SmokeLog::Get()->InstallSignalHandlers();
	}

	try
	{
		if (!autostart && ErrorWindow::CheckCrashReporter())
			return 0;

		if (commandline->HasArg("-h", "--help"))
		{
			std::cout << "SurrealEngine [--url=<mapname>] [--engineversion=X] [Path to game folder]\n";
			return 0;
		}

		int selectedGameIndex = autostart ? GameFolderSelection::AutoSelect() : LauncherWindow::ExecModal();
		if (selectedGameIndex >= 0)
		{
			GameLaunchInfo info = GameFolderSelection::GetLaunchInfo(selectedGameIndex);
			Engine engine(info);
			engine.Run();
		}
	}
	catch (const std::exception& e)
	{
		const std::string what = e.what();
		SmokeLog* smoke = SmokeLog::Get();
		if (smoke->IsOpen())
		{
			if (what.find("Unknown native function") != std::string::npos)
				smoke->MissingNative(what);
			else
				smoke->Exception(what);
			smoke->Fatal(what);
		}

		if (autostart)
		{
			// Unattended runs must not block on a modal window.
			if (!smoke->IsOpen())
				std::cerr << what << "\n";
			return 1;
		}
		ErrorWindow::ExecModal(e.what(), Logger::Get()->GetLog());
	}

	DeinitWidgetResources();
	if (SmokeLog::Get()->IsOpen() && !SmokeLog::Get()->IsFinished())
		SmokeLog::Get()->Summary();
	return 0;
}
