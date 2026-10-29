
#include "Precomp.h"
#include "Utils/Exception.h"
#include "Utils/Logger.h"
#include "Utils/CommandLine.h"
#include "GameApp.h"
#include "GameFolder.h"
#include "Engine.h"
#include "VM/Frame.h"
#include "UI/WidgetResourceData.h"
#include "UI/ErrorWindow/ErrorWindow.h"
#include "UI/Launcher/LauncherWindow.h"
#include "Utils/File.h"
#include "Utils/UserDirs.h"
#include <filesystem>
#include <stdexcept>
#include <iostream>

namespace
{
	// The guard's message when output lies inside a folder named on the command line, else empty. With --autostart the
	// first folder is the game that runs; without it the launcher may pick any of them. The launcher's saved list of
	// folders is not consulted.
	std::string RefusalInsideNamedFolders(const std::string& output)
	{
		for (const std::string& item : commandline->GetItems())
		{
			std::error_code ec;
			if (!std::filesystem::is_directory(item, ec))
				continue;

			try
			{
				WritePolicy::checkAgainst(item, output);
			}
			catch (const std::runtime_error& e)
			{
				return e.what();
			}
		}
		return {};
	}
}

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

		// Opening creates or truncates the file, so a log inside a game folder must be refused before it is opened
		const std::string refusal = RefusalInsideNamedFolders(path);
		if (!refusal.empty())
		{
			std::cerr << refusal << "\n";
			return 2;
		}

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
				"  --trace-script=<Class>[.<Function>][,...]\n"
				"                       Log each script call made on objects of these classes or their\n"
				"                       subclasses, or only the named function of such objects\n"
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

		// Likewise a malformed --trace-script
		TraceScriptResult traceScript;
		if (commandline->HasArg("", "--trace-script"))
		{
			traceScript = ParseTraceScript(commandline->GetArg("", "--trace-script"));
			if (!traceScript.ok())
			{
				std::cerr << "SurrealEngine: invalid --trace-script: " << traceScript.error << "\n";
				return 2;
			}
		}

		// Like --exec, a --user-dir that names nothing or lies inside a game folder named on the command line stops
		// the run before the launcher or any game folder is touched
		if (commandline->HasArg("", "--user-dir"))
		{
			std::string userDir;
			try
			{
				userDir = UserDirs::resolveUserDirArg(commandline->GetArg("", "--user-dir")).string();
			}
			catch (const std::exception& e)
			{
				std::cerr << "SurrealEngine: invalid --user-dir: " << e.what() << "\n";
				return 2;
			}

			const std::string refusal = RefusalInsideNamedFolders(userDir);
			if (!refusal.empty())
			{
				std::cerr << refusal << "\n";
				return 2;
			}
		}

		int selectedGameIndex = autostart ? GameFolderSelection::AutoSelect() : LauncherWindow::ExecModal();
		if (selectedGameIndex >= 0)
		{
			GameLaunchInfo info = GameFolderSelection::GetLaunchInfo(selectedGameIndex);
			info.execSchedule = std::move(execSchedule.items);

			// A game picked in the launcher was not known when the command line was checked, so look at the output once more.
			// A smoke log was opened by then and stays; the run still ends here, before any package loads.
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

			for (const TraceScriptEntry& entry : traceScript.entries)
				Frame::TraceEntries.push_back({ NameString(entry.cls), NameString(entry.function) });

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
