#include "Precomp.h"
#include "TestRunner.h"
#include "Engine.h"
#include <string>

// These cover the name a travelling player's record is stored under. Whether a login finds the record
// under that name needs a running engine and is checked by the three-map chains of Tools/smoke.

TEST(TravelRecordNameIsTheUrlNameWhenTheUrlHasOne)
{
	// The URL's name is the one the next login looks up first, so it wins over both of the others
	CHECK_EQ(TravelRecordName("Player", "Taylor", "Player1"), "Player");
	CHECK_EQ(TravelRecordName("Player", "", "Player1"), "Player");
	CHECK_EQ(TravelRecordName("Player", "Taylor", ""), "Player");
}

TEST(TravelRecordNameIsTheIniNameWhenTheUrlHasNone)
{
	// A partial travel empties the URL's name, and the login then reads the ini's
	CHECK_EQ(TravelRecordName("", "Taylor", "Player1"), "Taylor");
	CHECK_EQ(TravelRecordName("", "Taylor", ""), "Taylor");
}

TEST(TravelRecordNameIsTheReplicationNameWhenTheUrlAndIniHaveNone)
{
	CHECK_EQ(TravelRecordName("", "", "Player1"), "Player1");
}

TEST(TravelRecordNameIsEmptyWhenAllThreeAre)
{
	CHECK_EQ(TravelRecordName("", "", ""), "");
}

TEST(TravelRecordNameIsTheFirstNonEmptyOfTheThreeInOrder)
{
	// Every combination of present and absent, so that no source is skipped or taken out of order
	for (int mask = 0; mask < 8; mask++)
	{
		const std::string url = (mask & 1) ? "url" : "";
		const std::string ini = (mask & 2) ? "ini" : "";
		const std::string replication = (mask & 4) ? "replication" : "";

		std::string expected;
		for (const std::string* name : { &url, &ini, &replication })
		{
			if (!name->empty())
			{
				expected = *name;
				break;
			}
		}

		CHECK_EQ(TravelRecordName(url, ini, replication), expected);
	}
}
