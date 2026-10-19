#include "Precomp.h"
#include "TestRunner.h"
#include "Utils/UTF16.h"

TEST(NarrowUnicodeAscii)
{
	CHECK_EQ(narrow_utf16(L"Nali"), std::string("Nali"));
}

TEST(NarrowUnicodeKeepsBytesBelow256)
{
	// U+00E9 is the byte 0xE9: the narrowing keeps the code unit, it does not re-encode it.
	std::string expected = "caf";
	expected.push_back('\xe9');
	CHECK_EQ(narrow_utf16(L"café"), expected);
}

TEST(NarrowUnicodeReplacesWideUnits)
{
	// U+FD92 has the low byte 0x92; only a unit below 256 may pass as a byte.
	CHECK_EQ(narrow_utf16(L"aﶒb"), std::string("a?b"));
}

TEST(NarrowUnicodeReplacesEachSurrogateHalf)
{
	CHECK_EQ(narrow_utf16(L"\xd83d\xde00"), std::string("??"));
}

TEST(NarrowUnicodeEmpty)
{
	CHECK_EQ(narrow_utf16(L""), std::string());
}

TEST(NarrowUnicodeFirstUnitAboveByte)
{
	CHECK_EQ(narrow_utf16(L"Ā"), std::string("?"));
}
