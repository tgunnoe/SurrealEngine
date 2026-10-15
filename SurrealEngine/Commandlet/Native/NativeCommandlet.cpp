
#include "Precomp.h"
#include "NativeCommandlet.h"
#include "DebuggerApp.h"
#include "Engine.h"
#include "NativeCppUpdater.h"
#include "NativeObjExtractor.h"
#include "NativeFuncExtractor.h"
#include "NativeCppGenerator.h"
#include "Utils/File.h"

NativeCommandlet::NativeCommandlet()
{
	SetLongFormName("native");
	SetShortDescription("Extract native info or assemble code from native info");
}

void NativeCommandlet::OnCommand(DebuggerApp* console, const std::string& args)
{
	if (console->launchinfo.gameRootFolder.empty())
	{
		console->WriteOutput("Root Folder section of LaunchInfo is empty!");
		return;
	}

	if (args == "extractfuncs")
	{
		Engine engine(console->launchinfo);
		std::string path = console->launchinfo.gameExecutableName + "-" + console->launchinfo.gameVersionString + "-Natives.json";
		File::write_all_text(path, NativeFuncExtractor::Run(engine.packages.get()));
	}
	else if (args == "coverage")
	{
		// Reuse the debugger's engine when there is one; the registry is rebuilt per engine either way.
		std::unique_ptr<Engine> ownEngine;
		if (!engine)
			ownEngine = std::make_unique<Engine>(console->launchinfo);

		Array<std::string> skipped;
		Array<std::string> missing = NativeFuncExtractor::FindMissing(engine->packages.get(), skipped);
		std::string text;
		for (const std::string& line : missing)
			text += line + "\n";
		text += "MISSING " + std::to_string(missing.size()) + "\n";

		console->WriteOutput("GAME " + console->launchinfo.gameExecutableName + "-" + console->launchinfo.gameVersionString + NewLine());
		for (const std::string& pkg : skipped)
			console->WriteOutput("SKIPPED " + pkg + NewLine());
		if (console->OutFile.empty())
			console->WriteOutput(text);
		else
			File::write_all_text(console->OutFile, text);
	}
	else if (args == "extractprops")
	{
		Engine engine(console->launchinfo);
		std::string path = console->launchinfo.gameExecutableName + "-" + console->launchinfo.gameVersionString + "-Properties.json";
		File::write_all_text(path, NativeObjExtractor::Run(engine.packages.get()));
	}
	else if (args == "update")
	{
		NativeCppUpdater updater(console);
		updater.Run();
	}
	else if (args == "oldgenerator")
	{
		NativeCppGenerator::Run();
	}
	else
	{
		console->WriteOutput("Unknown command " + args + NewLine());
	}
}

void NativeCommandlet::OnPrintHelp(DebuggerApp* console)
{
	console->WriteOutput("Syntax: native extractfuncs" + NewLine());
	console->WriteOutput("Syntax: native coverage" + NewLine());
	console->WriteOutput("Syntax: native extractprops" + NewLine());
	console->WriteOutput("Syntax: native update" + NewLine());
}
