#include "TestRunner.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>

std::vector<TestCase>& TestRegistry()
{
	static std::vector<TestCase> registry;
	return registry;
}

static std::set<std::string> LoadExpectedFailures(const std::string& path)
{
	std::ifstream file(path);
	if (!file)
		throw std::runtime_error("Cannot open expected-failure list: " + path);

	std::set<std::string> names;
	std::string line;
	while (std::getline(file, line))
	{
		size_t comment = line.find('#');
		if (comment != std::string::npos)
			line.resize(comment);
		size_t begin = line.find_first_not_of(" \t\r");
		if (begin == std::string::npos)
			continue;
		size_t end = line.find_last_not_of(" \t\r");
		names.insert(line.substr(begin, end - begin + 1));
	}
	return names;
}

int main(int argc, char** argv)
{
	std::string testName;
	std::string xfailPath;
	bool list = false;
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--list"))
			list = true;
		else if (!strcmp(argv[i], "--test") && i + 1 < argc)
			testName = argv[++i];
		else if (!strcmp(argv[i], "--xfail-file") && i + 1 < argc)
			xfailPath = argv[++i];
		else
		{
			fprintf(stderr, "usage: %s [--list] --test <name> [--xfail-file <path>]\n", argv[0]);
			return 2;
		}
	}

	if (list)
	{
		for (const TestCase& test : TestRegistry())
			printf("%s\n", test.Name);
		return 0;
	}

	if (testName.empty())
	{
		fprintf(stderr, "no test selected; use --test <name> or --list\n");
		return 2;
	}

	const TestCase* selected = nullptr;
	for (const TestCase& test : TestRegistry())
	{
		if (testName == test.Name)
			selected = &test;
	}
	if (!selected)
	{
		fprintf(stderr, "unknown test: %s\n", testName.c_str());
		return 2;
	}

	bool expectedFailure = false;
	try
	{
		if (!xfailPath.empty())
			expectedFailure = LoadExpectedFailures(xfailPath).count(testName) != 0;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "%s\n", e.what());
		return 2;
	}

	std::string failure;
	bool failedByCheck = false;
	try
	{
		selected->Body();
	}
	catch (const TestFailure& f)
	{
		failure = f.Message;
		failedByCheck = true;
	}
	catch (const std::exception& e)
	{
		failure = std::string("unexpected exception: ") + e.what();
	}

	if (failure.empty())
	{
		if (expectedFailure)
		{
			printf("XPASS %s: listed as an expected failure but passed; remove it from the list\n", testName.c_str());
			return 1;
		}
		printf("PASS %s\n", testName.c_str());
		return 0;
	}

	// Only a failed check may be an expected failure; an exception means the test is broken.
	if (expectedFailure && failedByCheck)
	{
		printf("XFAIL %s: %s\n", testName.c_str(), failure.c_str());
		return 0;
	}
	printf("FAIL %s: %s\n", testName.c_str(), failure.c_str());
	return 1;
}
