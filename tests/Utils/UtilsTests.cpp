#include "Precomp.h"
#include "TestRunner.h"
#include "Utils/CommandLine.h"
#include "Utils/File.h"
#include "Utils/SHA1Sum.h"
#include "Package/IniFile.h"
#include <filesystem>

TEST(CommandLineShortAndLongForms)
{
	CommandLine cmd({ "-w=640", "--height=480", "-fullscreen", "--name=Unreal Gold", "map.unr", "extra" });

	CHECK(cmd.HasArg("-w", "--width"));
	CHECK(cmd.HasArg("-h", "--height"));
	CHECK(cmd.HasArg("-fullscreen", ""));
	CHECK(!cmd.HasArg("-x", "--missing"));
	CHECK(!cmd.HasArg("", ""));

	CHECK_EQ(cmd.GetArg("-w", "--width"), std::string("640"));
	CHECK_EQ(cmd.GetArg("-h", "--height"), std::string("480"));
	CHECK_EQ(cmd.GetArg("", "--name"), std::string("Unreal Gold"));
	CHECK_EQ(cmd.GetArg("-x", "--missing", "fallback"), std::string("fallback"));
	CHECK_EQ(cmd.GetArg("-fullscreen", ""), std::string());
}

TEST(CommandLineNumericArgs)
{
	CommandLine cmd({ "-w=640", "--scale=1.5", "--bad=abc" });

	CHECK_EQ(cmd.GetArgInt("-w", "--width"), 640);
	CHECK_EQ(cmd.GetArgInt("-x", "--missing", 7), 7);
	CHECK_EQ(cmd.GetArgInt("", "--bad", 7), 0); // present but unparsable: atoi semantics, not the default
	CHECK_EQ(cmd.GetArgFloat("", "--scale"), 1.5f);
	CHECK_EQ(cmd.GetArgDouble("", "--scale"), 1.5);
	CHECK_EQ(cmd.GetArgDouble("", "--missing", 2.5), 2.5);
}

TEST(CommandLineItems)
{
	CommandLine cmd({ "map.unr", "-w=640", "?entry=Start", "-", "last" });

	// Options never appear as items; a lone "-" and the "?" travel syntax are positional.
	const Array<std::string>& items = cmd.GetItems();
	CHECK_EQ(items.size(), (size_t)4);
	CHECK_EQ(items[0], std::string("map.unr"));
	CHECK_EQ(items[1], std::string("?entry=Start"));
	CHECK_EQ(items[2], std::string("-"));
	CHECK_EQ(items[3], std::string("last"));
}

TEST(Sha1OfKnownBuffers)
{
	// FIPS 180 test vectors
	TempFile abc("sha1-abc");
	File::write_all_text(abc.Path, "abc");
	CHECK_EQ(SHA1Sum::of_file(abc.Path), std::string("a9993e364706816aba3e25717850c26c9cd0d89d"));

	TempFile empty("sha1-empty");
	File::write_all_text(empty.Path, "");
	CHECK_EQ(SHA1Sum::of_file(empty.Path), std::string("da39a3ee5e6b4b0d3255bfef95601890afd80709"));

	TempFile longer("sha1-448bits");
	File::write_all_text(longer.Path, "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
	CHECK_EQ(SHA1Sum::of_file(longer.Path), std::string("84983e441c3bd26ebaae4aa1f95129e5e54670f1"));
}

TEST(Sha1OfMissingFileIsEmpty)
{
	TempFile missing("sha1-missing");
	CHECK_EQ(SHA1Sum::of_file(missing.Path), std::string());
}

TEST(IniFileParse)
{
	TempFile ini("parse.ini");
	File::write_all_text(ini.Path,
		"; a comment\n"
		"[Engine.Engine]\n"
		"GameRenderDevice=OpenGLDrv.OpenGLRenderDevice\n"
		"Width  =  1024\n"
		"\n"
		"[Other]\n"
		"Paths=../System/*.u\n"
		"Paths=../Maps/*.unr\n");

	IniFile file(ini.Path);
	CHECK_EQ(file.GetValue("Engine.Engine", "GameRenderDevice"), std::string("OpenGLDrv.OpenGLRenderDevice"));
	CHECK_EQ(file.GetValue("Engine.Engine", "Width"), std::string("1024"));
	CHECK_EQ(file.GetValue("engine.engine", "WIDTH"), std::string("1024")); // names are case-insensitive
	CHECK_EQ(file.GetValue("Engine.Engine", "Missing", "dflt"), std::string("dflt"));
	CHECK_EQ(file.GetValue("NoSuchSection", "Width", "dflt"), std::string("dflt"));

	Array<std::string> paths = file.GetValues("Other", "Paths");
	CHECK_EQ(paths.size(), (size_t)2);
	CHECK_EQ(paths[0], std::string("../System/*.u"));
	CHECK_EQ(paths[1], std::string("../Maps/*.unr"));

	CHECK_EQ(file.GetKeys("Engine.Engine").size(), (size_t)2);
}

TEST(IniFileRoundTrip)
{
	TempFile source("roundtrip-source.ini");
	TempFile saved("roundtrip-saved.ini");
	File::write_all_text(source.Path, "[Section]\nKeep=1\n");

	IniFile file(source.Path);
	file.SetValue("Section", "Added", "two words");
	file.SetValue("Fresh", "Key", "v");
	file.SetValues("Fresh", "List", { "a", "b", "c" });
	file.SaveTo(saved.Path);

	IniFile reloaded(saved.Path);
	CHECK_EQ(reloaded.GetValue("Section", "Keep"), std::string("1"));
	CHECK_EQ(reloaded.GetValue("Section", "Added"), std::string("two words"));
	CHECK_EQ(reloaded.GetValue("Fresh", "Key"), std::string("v"));

	Array<std::string> list = reloaded.GetValues("Fresh", "List");
	CHECK_EQ(list.size(), (size_t)3);
	CHECK_EQ(list[0], std::string("a"));
	CHECK_EQ(list[2], std::string("c"));
}
