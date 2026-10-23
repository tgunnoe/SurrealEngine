#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

// The tokenizer reads a text as a plain string and touches no engine object, so the tag
// language can be exercised without game data. UDXTextParser::ProcessText is the thin
// method that feeds it the text of an ExtString and stores the token in script properties.

enum class DeusExTextTags : uint8_t
{
	TT_Text,
	TT_File,
	TT_Email,
	TT_Note,
	TT_EndNote,
	TT_Goal,
	TT_EndGoal,
	TT_Comment,
	TT_EndComment,
	TT_PlayerName,
	TT_PlayerFirstName,
	TT_NewPage,
	TT_CenterText,
	TT_LeftJustify,
	TT_RightJustify,
	TT_DefaultColor,
	TT_TextColor,
	TT_RevertColor,
	TT_NewParagraph,
	TT_Bold,
	TT_EndBold,
	TT_Underline,
	TT_EndUnderline,
	TT_Italics,
	TT_EndItalics,
	TT_Graphic,
	TT_Font,
	TT_Label,
	TT_OpenBracket,
	TT_CloseBracket,
	TT_None
};

// One token of a text. Only the fields of its tag are filled; the colour is set by a colour tag.
struct TextToken
{
	DeusExTextTags tag = DeusExTextTags::TT_Text;
	std::string text;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	std::string emailName, emailSubject, emailFrom, emailTo, emailCC;
	std::string fileName, fileDescription;
	size_t pos = 0; // where the next token starts
};

namespace DeusExTextDetail
{
	inline void EatWhitespace(const std::string& text, size_t& pos)
	{
		size_t len = text.size();
		while (pos < len && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\r' || text[pos] == '\n'))
			pos++;
	}

	inline bool ReadTagName(const std::string& text, size_t& pos, std::string& tagname)
	{
		tagname.clear();

		size_t len = text.size();
		while (pos < len && ((text[pos] >= 'a' && text[pos] <= 'z') || (text[pos] >= 'A' && text[pos] <= 'Z') || text[pos] == '/'))
		{
			tagname += text[pos];
			pos++;
		}

		if (tagname.empty() || pos == len)
			return false;
		return true;
	}

	inline bool ReadChars(const std::string& text, size_t& pos, const std::string& chars)
	{
		if (pos + chars.size() >= text.size() || text.substr(pos, chars.size()) != chars)
			return false;
		pos += chars.size();
		return true;
	}

	inline bool ReadInteger(const std::string& text, size_t& pos, int value)
	{
		std::string v;
		size_t len = text.size();
		while (pos < len && text[pos] >= '0' && text[pos] <= '9')
		{
			v += text[pos];
			pos++;
		}
		if (v.empty())
			return false;
		value = std::atoi(v.c_str());
		return true;
	}

	inline bool ReadTagColor(const std::string& text, size_t& pos, TextToken& token)
	{
		if (!ReadChars(text, pos, "="))
			return false;
		if (!ReadInteger(text, pos, token.r))
			return false;
		if (!ReadChars(text, pos, ","))
			return false;
		if (!ReadInteger(text, pos, token.g))
			return false;
		if (!ReadChars(text, pos, ","))
			return false;
		if (!ReadInteger(text, pos, token.b))
			return false;
		token.a = 255;
		return true;
	}

	inline bool ReadTextUntil(const std::string& text, size_t& pos, std::string& value, char endChar)
	{
		value.clear();
		size_t len = text.size();
		while (pos < len && text[pos] != endChar)
		{
			value += text[pos];
			pos++;
		}
		return pos != len;
	}

	inline bool ReadTagFile(const std::string& text, size_t& pos, std::string& filename, std::string& filedescription)
	{
		if (!ReadTextUntil(text, pos, filename, ',') || !ReadChars(text, pos, ","))
			return false;
		return ReadTextUntil(text, pos, filedescription, '>');
	}

	inline bool ReadTagEmail(const std::string& text, size_t& pos, std::string& emailName, std::string& emailSubject, std::string& emailFrom, std::string& emailTo, std::string& emailCC)
	{
		if (!ReadTextUntil(text, pos, emailName, ',') || !ReadChars(text, pos, ","))
			return false;
		if (!ReadTextUntil(text, pos, emailSubject, ',') || !ReadChars(text, pos, ","))
			return false;
		if (!ReadTextUntil(text, pos, emailFrom, ',') || !ReadChars(text, pos, ","))
			return false;
		if (!ReadTextUntil(text, pos, emailTo, ',') || !ReadChars(text, pos, ","))
			return false;
		return ReadTextUntil(text, pos, emailCC, '>');
	}

	inline bool ReadText(const std::string& text, size_t& pos, std::string& value)
	{
		EatWhitespace(text, pos);

		value.clear();
		size_t len = text.size();
		while (pos < len && text[pos] != '<')
		{
			value += text[pos];
			pos++;
		}

		// Eat any whitespace at the end
		while (!value.empty() && (value.back() == ' ' || value.back() == '\r' || value.back() == '\n'))
			value.pop_back();

		if (value.empty() || pos == len)
			return false;
		return true;
	}
}

/*
	Tokenizer function reading pseudo HTML text ala this:

	<DC=255,255,255>
	<P>We'll make this one easy for you.  To open the door, use the code:
	<P>
	<P><JC><B>0012</B>
	<P>
	<P>Got it?
	<P>
	<P>Jaime

	Reads the token that starts at pos, after any whitespace. Returns false at the end of the
	text, with out.pos after that whitespace. A tag that cannot be read is delivered as text:
	the rest of the text from the tag's start.
*/
inline bool NextToken(const std::string& text, size_t pos, const std::string& playerName, const std::string& playerFirstName, TextToken& out)
{
	using namespace DeusExTextDetail;

	out = TextToken();
	size_t len = text.size();

	// Eat whitespace
	EatWhitespace(text, pos);

	if (pos >= len) // EOF
	{
		out.pos = pos;
		return false;
	}

	size_t tagstart = pos;
	if (!ReadChars(text, pos, "<"))
	{
		if (ReadText(text, pos, out.text))
		{
			out.tag = DeusExTextTags::TT_Text;
			out.pos = pos;
			return true;
		}
	}
	else
	{
		std::string tagname;
		if (ReadTagName(text, pos, tagname))
		{
			if (tagname == "DC" && ReadTagColor(text, pos, out) && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_DefaultColor;
				out.pos = pos;
				return true;
			}
			else if (tagname == "P" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_NewParagraph;
				out.pos = pos;
				return true;
			}
			else if (tagname == "B" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_Bold;
				out.pos = pos;
				return true;
			}
			else if (tagname == "/B" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_EndBold;
				out.pos = pos;
				return true;
			}
			else if (tagname == "I" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_Italics;
				out.pos = pos;
				return true;
			}
			else if (tagname == "/I" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_EndItalics;
				out.pos = pos;
				return true;
			}
			else if (tagname == "U" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_Underline;
				out.pos = pos;
				return true;
			}
			else if (tagname == "/U" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_EndUnderline;
				out.pos = pos;
				return true;
			}
			else if (tagname == "JC" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_None; // What does <JC> mean? Seems to be used for headlines
				out.pos = pos;
				return true;
			}
			else if (tagname == "JR" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_None; // What does <JR> mean? Seems to be used for signatures
				out.pos = pos;
				return true;
			}
			else if (tagname == "PLAYERNAME" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_PlayerName;
				out.text = playerName;
				out.pos = pos;
				return true;
			}
			else if (tagname == "PLAYERFIRSTNAME" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_PlayerFirstName;
				out.text = playerFirstName;
				out.pos = pos;
				return true;
			}
			else if (tagname == "COMMENT" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_Comment;
				out.pos = pos;
				return true;
			}
			else if (tagname == "/COMMENT" && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_EndComment;
				out.pos = pos;
				return true;
			}
			else if (tagname == "FILE" && ReadTagFile(text, pos, out.fileName, out.fileDescription) && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_File;
				out.pos = pos;
				return true;
			}
			else if (tagname == "EMAIL" && ReadTagEmail(text, pos, out.emailName, out.emailSubject, out.emailFrom, out.emailTo, out.emailCC) && ReadChars(text, pos, ">"))
			{
				out.tag = DeusExTextTags::TT_File;
				out.pos = pos;
				return true;
			}
		}
	}

	// Parse error. Dump what we got as text for debugging
	out = TextToken();
	out.tag = DeusExTextTags::TT_Text;
	out.text = text.substr(tagstart);
	out.pos = len;
	return true;
}
