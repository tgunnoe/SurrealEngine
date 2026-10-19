#include "Precomp.h"
#include "TestRunner.h"
#include "UnrealURL.h"

// These cover how a URL string is split into its map, its options and its teleport tag, how options are
// looked up by name, and which map name a saved game records.

namespace
{
	// The options in the order they were added, joined with '|' so that a test compares them as one text
	std::string OptionsOf(const UnrealURL& url)
	{
		std::string text;
		for (const std::string& option : url.Options)
		{
			if (!text.empty())
				text += "|";
			text += option;
		}
		return text;
	}
}

TEST(UnrealURLKeepsEveryOptionAfterTheMap)
{
	// The controls: a lone option, and a map without options, were always right
	CHECK_EQ(OptionsOf(UnrealURL("?load=9")), std::string("load=9"));
	CHECK_EQ(OptionsOf(UnrealURL("Vortex2")), std::string(""));
	CHECK_EQ(UnrealURL("Vortex2").Map, std::string("Vortex2"));

	CHECK_EQ(OptionsOf(UnrealURL("?a=1?b=2")), std::string("a=1|b=2"));
	CHECK_EQ(OptionsOf(UnrealURL("Vortex2?a=1?b=2?c")), std::string("a=1|b=2|c"));

	// Spaces around the URL are trimmed before it is split
	const UnrealURL padded("  Vortex2?a=1?b=2  ");
	CHECK_EQ(padded.Map, std::string("Vortex2"));
	CHECK_EQ(OptionsOf(padded), std::string("a=1|b=2"));
}

TEST(UnrealURLKeepsThreeOptionsAndFindsThemByName)
{
	const UnrealURL url("Vortex2?Name=Player?load=12?Class=x");

	CHECK_EQ(url.Map, std::string("Vortex2"));
	CHECK_EQ(OptionsOf(url), std::string("Name=Player|load=12|Class=x"));
	CHECK_EQ(url.GetOption("load"), std::string("12"));
	CHECK_EQ(url.GetOption("Class"), std::string("x"));
	CHECK(url.HasOption("Class"));
}

TEST(UnrealURLKeepsAnOptionAfterTheTeleportTag)
{
	// The control: a tag alone was always right, with either separator
	CHECK_EQ(UnrealURL("Map#tag").Portal, std::string("tag"));
	CHECK_EQ(UnrealURL("Map/tag").Portal, std::string("tag"));

	const UnrealURL hash("Map#tag?opt=1");
	CHECK_EQ(hash.Map, std::string("Map"));
	CHECK_EQ(hash.Portal, std::string("tag"));
	CHECK_EQ(OptionsOf(hash), std::string("opt=1"));

	const UnrealURL slash("Map/tag?opt=1");
	CHECK_EQ(slash.Map, std::string("Map"));
	CHECK_EQ(slash.Portal, std::string("tag"));
	CHECK_EQ(OptionsOf(slash), std::string("opt=1"));
}

TEST(UnrealURLKeepsTheTeleportTagAfterAnOption)
{
	// The control: an option and a tag after the last option's separator were kept before
	CHECK_EQ(UnrealURL("Map?opt=1").Portal, std::string(""));

	const UnrealURL hash("Map?opt=1#tag");
	CHECK_EQ(hash.Map, std::string("Map"));
	CHECK_EQ(hash.Portal, std::string("tag"));
	CHECK_EQ(OptionsOf(hash), std::string("opt=1"));

	const UnrealURL slash("Map?opt=1/tag");
	CHECK_EQ(slash.Map, std::string("Map"));
	CHECK_EQ(slash.Portal, std::string("tag"));
	CHECK_EQ(OptionsOf(slash), std::string("opt=1"));
}

TEST(UnrealURLKeepsEveryOptionWhereverTheTagSits)
{
	// Every place a tag can go among up to four options, with either separator. The expected options are
	// built from the same pieces as the text, not read back from the parser.
	for (int count = 0; count <= 4; count++)
	{
		for (int tagBefore = -1; tagBefore <= count; tagBefore++) // -1 for no tag, else the option it precedes
		{
			for (const char tagSeparator : { '#', '/' })
			{
				if (tagBefore < 0 && tagSeparator == '/')
					continue;

				std::string text = "Vortex2";
				std::string expectedOptions;
				for (int i = 0; i <= count; i++)
				{
					if (i == tagBefore)
						text += std::string(1, tagSeparator) + "tag";
					if (i == count)
						break;

					const std::string option = "opt" + std::to_string(i) + "=" + std::to_string(i);
					text += "?" + option;
					expectedOptions += (expectedOptions.empty() ? "" : "|") + option;
				}

				const UnrealURL url(text);
				const std::string expectedTag = tagBefore < 0 ? "" : "tag";
				CHECK_MSG(url.Map == "Vortex2" && url.Portal == expectedTag && OptionsOf(url) == expectedOptions,
					"'" + text + "' gave map '" + url.Map + "', tag '" + url.Portal + "', options '" + OptionsOf(url) + "'");
			}
		}
	}
}

TEST(UnrealURLHasOptionIgnoresTheCaseOfTheName)
{
	const UnrealURL url("Vortex2?Name=Player");

	// The controls: GetOption has always matched without case, and a lower case name matched the stored key
	CHECK_EQ(url.GetOption("Name"), std::string("Player"));
	CHECK(url.HasOption("name"));

	CHECK(url.HasOption("Name"));
	CHECK(url.HasOption("NAME"));
	CHECK(UnrealURL("?Restart").HasOption("Restart"));
	CHECK(UnrealURL("?restart").HasOption("Restart"));

	// Another name is another option, whatever its case
	CHECK(!url.HasOption("Nam"));
	CHECK(!url.HasOption("Names"));
	CHECK(!url.HasOption("Player"));
	CHECK(!url.HasOption("Class"));
}

TEST(UnrealURLToStringRoundTripsAMapWithTwoOptions)
{
	// The control: one option was written whole
	CHECK_EQ(UnrealURL("Vortex2?Name=Player").ToString(), std::string("Vortex2?Name=Player"));

	CHECK_EQ(UnrealURL("Vortex2?Name=Player?load=12").ToString(), std::string("Vortex2?Name=Player?load=12"));
	CHECK_EQ(UnrealURL("Vortex2?a=1?b=2?c=3").ToString(), std::string("Vortex2?a=1?b=2?c=3"));
}

TEST(UnrealURLMapNameDropsTheMapExtension)
{
	// The controls: a name without the extension is the map name already
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2"), "unr"), std::string("Vortex2"));
	CHECK_EQ(MapNameOf(UnrealURL("01_NYC_UNATCOIsland"), "dx"), std::string("01_NYC_UNATCOIsland"));

	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.unr"), "unr"), std::string("Vortex2"));
	CHECK_EQ(MapNameOf(UnrealURL("01_NYC_UNATCOIsland.dx"), "dx"), std::string("01_NYC_UNATCOIsland"));
	// The brackets of a deathmatch map name belong to the name
	CHECK_EQ(MapNameOf(UnrealURL("DM-Deck16][.unr"), "unr"), std::string("DM-Deck16]["));
}

TEST(UnrealURLMapNameMatchesTheExtensionWithoutCase)
{
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.UNR"), "unr"), std::string("Vortex2"));
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.Unr"), "unr"), std::string("Vortex2"));
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.unr"), "UNR"), std::string("Vortex2"));
	// The case of the name itself is kept: the map loader corrects it against the file on the next load
	CHECK_EQ(MapNameOf(UnrealURL("vortex2.unr"), "unr"), std::string("vortex2"));
}

TEST(UnrealURLMapNameKeepsADotThatIsNotTheExtension)
{
	// The control: the extension is dropped only after it is told apart from the rest of the name
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.unr"), "unr"), std::string("Vortex2"));

	CHECK_EQ(MapNameOf(UnrealURL("Some.Map"), "unr"), std::string("Some.Map"));
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.unr.bak"), "unr"), std::string("Vortex2.unr.bak"));
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.dx"), "unr"), std::string("Vortex2.dx"));
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2unr"), "unr"), std::string("Vortex2unr"));
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.unrx"), "unr"), std::string("Vortex2.unrx"));
}

TEST(UnrealURLMapNameLeavesTheTagAndOptionsOut)
{
	// The control: the stripped name does not depend on what follows the map in the URL
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.unr"), "unr"), std::string("Vortex2"));

	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.unr#tag?Name=Player"), "unr"), std::string("Vortex2"));
	CHECK_EQ(MapNameOf(UnrealURL("Vortex2.unr?a=1?b=2"), "unr"), std::string("Vortex2"));
}
