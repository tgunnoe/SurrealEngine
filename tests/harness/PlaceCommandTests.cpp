#include "Precomp.h"
#include "TestRunner.h"
#include "Engine.h"
#include <initializer_list>
#include <string>

// These cover how the harness's "place" console command line becomes a location and an optional
// yaw, and the text of the smoke-log line that reports a placement. Moving the player needs a
// running engine and is checked by running maps.

namespace
{
	Array<std::string> Args(std::initializer_list<const char*> words)
	{
		Array<std::string> args;
		for (const char* word : words)
			args.push_back(word);
		return args;
	}

	const char* const Usage = "place takes '<x> <y> <z> [<yaw>]'";
}

TEST(HarnessPlaceParsesThreeNumbersAsALocationWithoutAYaw)
{
	PlaceParseResult r = ParsePlaceCommand(Args({ "place", "10", "20", "30" }));
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.place.location.x, 10.0f);
	CHECK_EQ(r.place.location.y, 20.0f);
	CHECK_EQ(r.place.location.z, 30.0f);
	CHECK(!r.place.hasYaw);

	// The command word is the caller's; the parser does not look at it
	CHECK(ParsePlaceCommand(Args({ "PLACE", "10", "20", "30" })).ok());
}

TEST(HarnessPlaceParsesFourNumbersAsALocationAndAYaw)
{
	PlaceParseResult r = ParsePlaceCommand(Args({ "place", "10", "20", "30", "38" }));
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.place.location.x, 10.0f);
	CHECK_EQ(r.place.location.y, 20.0f);
	CHECK_EQ(r.place.location.z, 30.0f);
	CHECK(r.place.hasYaw);
	CHECK_EQ(r.place.yawDegrees, 38.0f);
}

TEST(HarnessPlaceRefusesAnOperandThatIsNotANumberNamingIt)
{
	PlaceParseResult x = ParsePlaceCommand(Args({ "place", "east", "20", "30" }));
	CHECK(!x.ok());
	CHECK_EQ(x.error, std::string("place x 'east' is not a number"));

	PlaceParseResult y = ParsePlaceCommand(Args({ "place", "10", "12abc", "30" }));
	CHECK(!y.ok());
	CHECK_EQ(y.error, std::string("place y '12abc' is not a number"));

	PlaceParseResult z = ParsePlaceCommand(Args({ "place", "10", "20", "" }));
	CHECK(!z.ok());
	CHECK_EQ(z.error, std::string("place z '' is not a number"));

	PlaceParseResult yaw = ParsePlaceCommand(Args({ "place", "10", "20", "30", "north" }));
	CHECK(!yaw.ok());
	CHECK_EQ(yaw.error, std::string("place yaw 'north' is not a number"));

	// The first bad operand in the order x, y, z, yaw decides the refusal
	PlaceParseResult several = ParsePlaceCommand(Args({ "place", "10", "up", "down", "round" }));
	CHECK_EQ(several.error, std::string("place y 'up' is not a number"));
}

TEST(HarnessPlaceRefusesTheWrongNumberOfOperandsWithTheUsage)
{
	CHECK_EQ(ParsePlaceCommand(Array<std::string>()).error, std::string(Usage));
	CHECK_EQ(ParsePlaceCommand(Args({ "place" })).error, std::string(Usage));
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "10" })).error, std::string(Usage));
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "10", "20" })).error, std::string(Usage));
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "10", "20", "30", "40", "50" })).error, std::string(Usage));
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "10", "20", "30", "40", "50", "60" })).error, std::string(Usage));
}

TEST(HarnessPlaceParsesFractionsAndNegatives)
{
	PlaceParseResult r = ParsePlaceCommand(Args({ "place", "-2322.5", "0.25", "-380", "-90.5" }));
	CHECK_MSG(r.ok(), r.error);
	CHECK_EQ(r.place.location.x, -2322.5f);
	CHECK_EQ(r.place.location.y, 0.25f);
	CHECK_EQ(r.place.location.z, -380.0f);
	CHECK(r.place.hasYaw);
	CHECK_EQ(r.place.yawDegrees, -90.5f);
}

TEST(HarnessPlaceRefusesANumberThatIsNotFiniteNamingTheOperand)
{
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "inf", "20", "30" })).error, std::string("place x 'inf' is not a number"));
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "10", "nan", "30" })).error, std::string("place y 'nan' is not a number"));
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "10", "20", "-inf" })).error, std::string("place z '-inf' is not a number"));
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "10", "20", "30", "nan" })).error, std::string("place yaw 'nan' is not a number"));

	// Finite as a double but not as the float the location is stored in
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "1e39", "20", "30" })).error, std::string("place x '1e39' is not a number"));
	CHECK_EQ(ParsePlaceCommand(Args({ "place", "10", "20", "30", "1e39" })).error, std::string("place yaw '1e39' is not a number"));
}

TEST(HarnessPlaceFormatsTheLogLineWithOneDecimalPerNumber)
{
	CHECK_EQ(FormatPlaceLine(vec3(-2199.0f, 344.0f, -380.0f), vec3(-2199.0f, 344.0f, -440.0f), true, 38.0f),
		std::string("(-2199.0, 344.0, -380.0) -> (-2199.0, 344.0, -440.0) yaw 38.0"));
	CHECK_EQ(FormatPlaceLine(vec3(1.26f, -0.5f, 100.0f), vec3(3.0f, 4.5f, 6.0f), false, 90.0f),
		std::string("(1.3, -0.5, 100.0) -> (3.0, 4.5, 6.0)"));
	CHECK_EQ(FormatPlaceLine(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), true, -90.5f),
		std::string("(0.0, 0.0, 0.0) -> (0.0, 0.0, 0.0) yaw -90.5"));
}
