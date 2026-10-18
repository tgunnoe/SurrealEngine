
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
#include "Utils/UserDirs.h"
#include <stdexcept>
#include <iostream>

int GameApp::main(Array<std::string> args)
{
	SmokeLog::Get(); // Starts the --exit-after clock

	InitWidgetResources("dark");

	CommandLine cmd(args);
	commandline = &cmd;

	const bool autostart = commandline->HasArg("", "--autostart");
	std::string smokeLogPath;
	if (commandline->HasArg("", "--smoke-log"))
	{
		const std::string path = commandline->GetArg("", "--smoke-log");
		smokeLogPath = path;
		if (!SmokeLog::Get()->Open(path))
		{
			std::cerr << "Unable to open smoke log " << path << "\n";
			return 1;
		}
		// Without --autostart the crash reporter installs its own handlers, so ours would be replaced.
		if (autostart)
			SmokeLog::Get()->InstallSignalHandlers();
	}

	Array<ExecItem> unrunExec;

	try
	{
		if (!autostart && ErrorWindow::CheckCrashReporter())
			return 0;

		if (commandline->HasArg("-h", "--help"))
		{
			std::cout << "SurrealEngine [--url=<mapname>] [--engineversion=X] [Path to game folder]\n"
				"  --autostart          Skip the launcher and run the given game folder; write nothing to it\n"
				"  --user-dir=<path>    Keep ini files, saves and exports in this folder; with --autostart this\n"
				"                       also allows them to be written\n"
				"  --exit-after=<secs>  Quit the main loop this many seconds after process start\n"
				"  --exec=<secs>:<command>[;<secs>:<command>...]\n"
				"                       Run console commands this many seconds after the first login\n"
				"  --smoke-log=<path>   Write a structured, flushed event log for unattended runs\n";
			return 0;
		}

		// A malformed --exec stops the run here, before the launcher or any game folder is touched
		ExecScheduleResult execSchedule;
		if (commandline->HasArg("", "--exec"))
		{
			execSchedule = ParseExecSchedule(commandline->GetArg("", "--exec"));
			if (!execSchedule.ok())
			{
				std::cerr << "SurrealEngine: invalid --exec: " << execSchedule.error << "\n";
				return 2;
			}
		}

		// Like --exec, a --user-dir that names nothing stops the run before the launcher or any game folder is touched
		if (commandline->HasArg("", "--user-dir"))
		{
			try
			{
				UserDirs::resolveUserDirArg(commandline->GetArg("", "--user-dir"));
			}
			catch (const std::exception& e)
			{
				std::cerr << "SurrealEngine: invalid --user-dir: " << e.what() << "\n";
				return 2;
			}
		}

		int selectedGameIndex = autostart ? GameFolderSelection::AutoSelect() : LauncherWindow::ExecModal();
		if (selectedGameIndex >= 0)
		{
			GameLaunchInfo info = GameFolderSelection::GetLaunchInfo(selectedGameIndex);
			info.execSchedule = std::move(execSchedule.items);

			// Output the run was told to write that lands inside the game folder ends it before any package loads.
			// The smoke log was opened before a game was selected, so this is the first chance to look at it.
			WritePolicy::setForbiddenRoot(info.gameRootFolder);
			for (const std::string& output : { info.userDir, smokeLogPath })
			{
				if (output.empty())
					continue;
				try
				{
					WritePolicy::check(output);
				}
				catch (const std::runtime_error& e)
				{
					std::cerr << e.what() << "\n";
					return 2;
				}
			}

			Engine engine(info);
			engine.Run();
			unrunExec = engine.UnrunExecCommands();
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
	{
		for (const ExecItem& item : unrunExec)
			SmokeLog::Get()->Event("EXEC_SKIPPED", FormatExecItem(item));
		SmokeLog::Get()->Summary();
	}
	return 0;
}
