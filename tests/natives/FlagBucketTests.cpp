#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Extension/Flags/UFlagBase.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

// The flag base keeps its flags in 64 buckets picked from the flag's name. The bucket is written into a
// save with the flags and looked up again by the next process, so it has to come from the name's text
// and nothing a process decides for itself.

namespace
{
	std::vector<std::string> NameTexts(int count)
	{
		std::vector<std::string> texts;
		for (int i = 1; i <= count; i++)
			texts.push_back("Name" + std::to_string(i));
		return texts;
	}

	std::string Lower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return text;
	}

	std::string Upper(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)std::toupper(c); });
		return text;
	}

	int Bucket(const std::string& text)
	{
		return UFlagBase::BucketOf(NameString(text));
	}
}

TEST(FlagBucketIgnoresCase)
{
	std::set<int> buckets;
	for (const std::string& text : NameTexts(1000))
	{
		int bucket = Bucket(text);
		CHECK_EQ(Bucket(Lower(text)), bucket);
		CHECK_EQ(Bucket(Upper(text)), bucket);
		buckets.insert(bucket);
	}

	// A function that returned one fixed bucket would pass the checks above for nothing.
	CHECK(buckets.size() > 1);
}

TEST(FlagBucketIsBelowTheTableSize)
{
	std::set<int> buckets;
	for (const std::string& text : NameTexts(1000))
	{
		int bucket = Bucket(text);
		CHECK(bucket >= 0);
		CHECK(bucket < UFlagBase::HashTableSize);
		buckets.insert(bucket);
	}

	// Same reason: an out-of-range constant fails above, an in-range constant must fail here.
	CHECK(buckets.size() > 1);
}

TEST(FlagBucketSpreadsNamesOverTheTable)
{
	std::set<int> buckets;
	for (const std::string& text : NameTexts(1000))
		buckets.insert(Bucket(text));

	// A thousand names landing in a handful of buckets would turn every lookup into a list walk.
	CHECK(buckets.size() > (size_t)UFlagBase::HashTableSize / 2);
}

TEST(FlagBucketOfKnownNamesIsFixed)
{
	// Saves written by one build are read by another, so the bucket of a name must not change
	// between builds or between processes (it once followed the order in which a process first met
	// names). The expected values are FNV-1a over the upper-cased text, modulo 64, computed apart
	// from the engine. Changing the function means saved flags are no longer found.
	CHECK_EQ(Bucket("ProbeFlag"), 43);
	CHECK_EQ(Bucket("PlayerTraveling"), 52);
	CHECK_EQ(Bucket("MeetPaul_Played"), 18);
	CHECK_EQ(Bucket("Name1"), 53);
	CHECK_EQ(Bucket("a"), 12);
}
