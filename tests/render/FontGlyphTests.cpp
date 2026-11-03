#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Engine/Resources/UFont.h"

// These cover the lookup behind UFont::FindGlyph over a page list built by hand. FindGlyph(char)
// itself is private and only hands its argument on as an unsigned char; whether a font's pages
// really carry glyphs at the indices of the Latin-1 bytes is a property of the font data and
// is not checked here.

namespace
{
	// Every character takes its index over the whole list as StartU, with a size of 1, so that a hit
	// names the character it found. A miss comes back with a size of 0, and a hit on the wrong page
	// would show as a different StartU.
	Array<FontPage> MakePages(std::initializer_list<size_t> pageSizes)
	{
		Array<FontPage> pages;
		int next = 0;
		for (size_t pageSize : pageSizes)
		{
			FontPage page;
			page.Characters.resize(pageSize);
			for (FontCharacter& character : page.Characters)
			{
				character.StartU = next++;
				character.USize = 1;
			}
			pages.push_back(page);
		}
		return pages;
	}
}

TEST(FontGlyphAsciiLetterHitsPageOne)
{
	FontGlyph glyph = FindFontGlyph(MakePages({ 128, 128 }), static_cast<unsigned char>('A'));
	CHECK_EQ(glyph.StartU, 65);
}

TEST(FontGlyphLastAsciiByteHitsPageOne)
{
	FontGlyph glyph = FindFontGlyph(MakePages({ 128, 128 }), static_cast<unsigned char>(0x7F));
	CHECK_EQ(glyph.StartU, 127);
}

TEST(FontGlyphFirstHighByteHitsPageTwo)
{
	FontGlyph glyph = FindFontGlyph(MakePages({ 128, 128 }), static_cast<unsigned char>(0x80));
	CHECK_EQ(glyph.StartU, 128);
}

TEST(FontGlyphLatin1ByteHitsPageTwo)
{
	FontGlyph glyph = FindFontGlyph(MakePages({ 128, 128 }), static_cast<unsigned char>(0xE9));
	CHECK_EQ(glyph.StartU, 233);
}

TEST(FontGlyphLastByteHitsPageTwo)
{
	FontGlyph glyph = FindFontGlyph(MakePages({ 128, 128 }), static_cast<unsigned char>(0xFF));
	CHECK_EQ(glyph.StartU, 255);
}

// A byte read out of a std::string is a char, which is -23 here for 0xE9. The parameter has to turn
// it back into 233; a size_t or char parameter would see a huge index or a negative one and miss.
TEST(FontGlyphSignedCharMeansItsByteValue)
{
	char fromString = static_cast<char>(0xE9);
	FontGlyph glyph = FindFontGlyph(MakePages({ 128, 128 }), fromString);
	CHECK_EQ(glyph.StartU, 233);
}

// The two miss cases below pass against a lookup that finds nothing as well as against the real one.
// They are controls: a lookup that answered every index with some glyph would fail them.
TEST(FontGlyphByteBeyondThePagesMisses)
{
	FontGlyph glyph = FindFontGlyph(MakePages({ 128 }), static_cast<unsigned char>(0xE9));
	CHECK_EQ(glyph.USize, 0);
}

TEST(FontGlyphEmptyPageListMisses)
{
	FontGlyph glyph = FindFontGlyph(MakePages({}), static_cast<unsigned char>('A'));
	CHECK_EQ(glyph.USize, 0);
}
