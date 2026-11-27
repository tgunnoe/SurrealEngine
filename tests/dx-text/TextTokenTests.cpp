#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/DeusExText/TextTokenizer.h"
#include <string>
#include <vector>

// Datacubes, books and newspapers are texts written in a small tag language that the scripts
// read token by token. These tests feed the tokenizer literal texts and check each token as
// the script would receive it. Every literal that tests a tag carries content after it, so the
// last-byte rule (a tag that ends the text) cannot mask the claim being made.

namespace
{
	using Tag = DeusExTextTags;

	const char* const TagNames[] = {
		"TT_Text", "TT_File", "TT_Email", "TT_Note", "TT_EndNote", "TT_Goal", "TT_EndGoal", "TT_Comment",
		"TT_EndComment", "TT_PlayerName", "TT_PlayerFirstName", "TT_NewPage", "TT_CenterText", "TT_LeftJustify",
		"TT_RightJustify", "TT_DefaultColor", "TT_TextColor", "TT_RevertColor", "TT_NewParagraph", "TT_Bold",
		"TT_EndBold", "TT_Underline", "TT_EndUnderline", "TT_Italics", "TT_EndItalics", "TT_Graphic", "TT_Font",
		"TT_Label", "TT_OpenBracket", "TT_CloseBracket", "TT_None" };
	static_assert(sizeof(TagNames) / sizeof(TagNames[0]) == (size_t)Tag::TT_None + 1, "TagNames must list every tag");

	// The tag by name, so a failed check says which tag came back
	std::string Name(Tag tag)
	{
		return TagNames[(size_t)tag];
	}

	// Every token of a text, read the way ProcessText's loop reads them
	std::vector<TextToken> Tokenize(const std::string& text)
	{
		std::vector<TextToken> tokens;
		size_t pos = 0;
		TextToken token;
		while (NextToken(text, pos, "", "", token))
		{
			CHECK_MSG(token.pos > pos, "a token must advance past its start, at " + std::to_string(pos));
			tokens.push_back(token);
			pos = token.pos;
		}
		return tokens;
	}

	// The tokens as one line: each tag, with the text in parentheses when it carries any
	std::string Render(const std::vector<TextToken>& tokens)
	{
		std::string line;
		for (const TextToken& token : tokens)
		{
			if (!line.empty())
				line += " ";
			line += Name(token.tag);
			if (!token.text.empty())
				line += "(" + token.text + ")";
		}
		return line;
	}

	TextToken FirstToken(const std::string& text)
	{
		std::vector<TextToken> tokens = Tokenize(text);
		CHECK_MSG(!tokens.empty(), "no token read from: " + text);
		return tokens.front();
	}

	// The tokenizer's own example, with the line endings the shipped texts have
	const std::string Sample =
		"<DC=255,255,255>\r\n"
		"<P>We'll make this one easy for you.  To open the door, use the code:\r\n"
		"<P>\r\n"
		"<P><JC><B>0012</B>\r\n"
		"<P>\r\n"
		"<P>Got it?\r\n"
		"<P>\r\n"
		"<P>Jaime";

	const char* const SampleTokens =
		"TT_DefaultColor TT_NewParagraph TT_Text(We'll make this one easy for you.  To open the door, use the code:) "
		"TT_NewParagraph TT_NewParagraph TT_CenterText TT_Bold TT_Text(0012) TT_EndBold "
		"TT_NewParagraph TT_NewParagraph TT_Text(Got it?) TT_NewParagraph TT_NewParagraph TT_Text(Jaime)";
}

TEST(DxTextDefaultColorTagCarriesItsColor)
{
	std::vector<TextToken> tokens = Tokenize("<DC=255,200,100><P>Hello");
	CHECK(tokens.size() >= 2);
	CHECK_EQ(Name(tokens[0].tag), "TT_DefaultColor");
	CHECK_EQ((int)tokens[0].r, 255);
	CHECK_EQ((int)tokens[0].g, 200);
	CHECK_EQ((int)tokens[0].b, 100);
	CHECK_EQ((int)tokens[0].a, 255);

	// Only a colour tag sets a colour
	CHECK_EQ(Name(tokens[1].tag), "TT_NewParagraph");
	CHECK_EQ((int)tokens[1].r + tokens[1].g + tokens[1].b + tokens[1].a, 0);
}

TEST(DxTextEmailAndFileTagsCarryTheirFields)
{
	TextToken email = FirstToken("<EMAIL=Paul,Welcome,Paul Denton,JC Denton,><P>x");
	CHECK_EQ(Name(email.tag), "TT_Email");
	CHECK_EQ(email.emailName, "Paul");
	CHECK_EQ(email.emailSubject, "Welcome");
	CHECK_EQ(email.emailFrom, "Paul Denton");
	CHECK_EQ(email.emailTo, "JC Denton");
	CHECK_EQ(email.emailCC, "");

	TextToken file = FirstToken("<FILE=notes,My notes><P>x");
	CHECK_EQ(Name(file.tag), "TT_File");
	CHECK_EQ(file.fileName, "notes");
	CHECK_EQ(file.fileDescription, "My notes");
}

TEST(DxTextJustifyTagsAreDelivered)
{
	CHECK_EQ(Name(FirstToken("<JC><P>x").tag), "TT_CenterText");
	CHECK_EQ(Name(FirstToken("<JL><P>x").tag), "TT_LeftJustify");
	CHECK_EQ(Name(FirstToken("<JR><P>x").tag), "TT_RightJustify");
}

TEST(DxTextTagOnTheLastByteIsStillATag)
{
	CHECK_EQ(Render(Tokenize("Hello<P>")), "TT_Text(Hello) TT_NewParagraph");
	CHECK_EQ(Render(Tokenize("<B>x</B>")), "TT_Bold TT_Text(x) TT_EndBold");

	// Plain text that runs to the last byte is delivered whole
	CHECK_EQ(Render(Tokenize("Hello world")), "TT_Text(Hello world)");
}

TEST(DxTextSampleTokenSequence)
{
	CHECK_EQ(Render(Tokenize(Sample)), SampleTokens);
}

TEST(DxTextUnknownTagFallsBackToTheRestAsText)
{
	std::vector<TextToken> tokens = Tokenize("<LOG ERROR><P>x");
	CHECK_EQ(tokens.size(), (size_t)1);
	CHECK_EQ(Name(tokens[0].tag), "TT_Text");
	CHECK_EQ(tokens[0].text, "<LOG ERROR><P>x");
}

TEST(DxTextPlayerNameTagCarriesTheName)
{
	TextToken token;
	CHECK(NextToken("<PLAYERNAME><P>x", 0, "JC Denton", "JC", token));
	CHECK_EQ(Name(token.tag), "TT_PlayerName");
	CHECK_EQ(token.text, "JC Denton");

	CHECK(NextToken("<PLAYERFIRSTNAME><P>x", 0, "JC Denton", "JC", token));
	CHECK_EQ(Name(token.tag), "TT_PlayerFirstName");
	CHECK_EQ(token.text, "JC");
}

TEST(DxTextLoopVisitsEveryTokenOnceAndEndsAtTheLength)
{
	// ProcessText calls the tokenizer in this loop, and the script stops when it returns false
	size_t visited = 0;
	size_t pos = 0;
	TextToken token;
	while (NextToken(Sample, pos, "", "", token))
	{
		CHECK(token.pos > pos);
		CHECK(token.pos <= Sample.size());
		pos = token.pos;
		visited++;
	}
	CHECK_EQ(visited, (size_t)15);
	CHECK_EQ(token.pos, Sample.size());

	// Whatever the text is cut to, the loop terminates and ends at that length
	for (size_t length = 0; length <= Sample.size(); length++)
	{
		std::string cut = Sample.substr(0, length);
		pos = 0;
		while (NextToken(cut, pos, "", "", token))
		{
			CHECK_MSG(token.pos > pos, "stuck at " + std::to_string(pos) + " in a text cut to " + std::to_string(length));
			pos = token.pos;
		}
		CHECK_EQ(token.pos, cut.size());
	}
}
