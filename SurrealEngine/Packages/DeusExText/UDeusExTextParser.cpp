
#include "Precomp.h"
#include "UDeusExTextParser.h"
#include "Packages/Extension/UExtString.h"
#include "Package/Package.h"
#include "Package/PackageManager.h"
#include "Engine.h"
#include "Utils/Logger.h"

bool UDXTextParser::OpenText(NameString textName, std::string textPackage)
{
	CloseText();
	if (textName.IsNone())
		textName = "DeusExQuotes"; // Not correct, but it handles empty strings just fine.
	if (textPackage.empty())
		textPackage = "DeusExText";
	textObject = UObject::Cast<UDXExtString>(engine->packages->GetPackage(textPackage)->GetUObject("ExtString", textName));
	Text() = textObject ? 1 : 0;
	return textObject;
}

void UDXTextParser::CloseText()
{
	textObject = nullptr;
	Text() = 0;
	TextPos() = 0;
	TagEndPos() = 0;
}

bool UDXTextParser::ProcessText()
{
	if (!textObject)
		return false;

	TextToken token;
	bool more = NextToken(textObject->Text(), (size_t)TextPos(), PlayerName(), PlayerFirstName(), token);
	TextPos() = (int)token.pos;
	if (!more) // EOF
		return false;

	LastTag() = token.tag;
	switch (token.tag)
	{
	case DeusExTextTags::TT_Text:
	case DeusExTextTags::TT_PlayerName:
	case DeusExTextTags::TT_PlayerFirstName:
		LastText() = token.text;
		break;
	case DeusExTextTags::TT_DefaultColor:
		DefaultColor().R = token.r;
		DefaultColor().G = token.g;
		DefaultColor().B = token.b;
		DefaultColor().A = token.a;
		LastColor() = DefaultColor();
		break;
	case DeusExTextTags::TT_File:
		LastFileName() = token.fileName;
		LastFileDescription() = token.fileDescription;
		break;
	case DeusExTextTags::TT_Email:
		LastEmailName() = token.emailName;
		LastEmailSubject() = token.emailSubject;
		LastEmailFrom() = token.emailFrom;
		LastEmailTo() = token.emailTo;
		LastEmailCC() = token.emailCC;
		break;
	default:
		break;
	}
	return true;
}

bool UDXTextParser::IsEOF()
{
	return !textObject || TextPos() == textObject->Text().size();
}

std::string UDXTextParser::GetText()
{
	return LastText();
}

void UDXTextParser::GotoLabel(const std::string& label)
{
	LogUnimplemented("DeusExTextParser.GotoLabel()");
}

uint8_t UDXTextParser::GetTag()
{
	return static_cast<uint8_t>(LastTag());
}

NameString UDXTextParser::GetName()
{
	if (LastTag() == DeusExTextTags::TT_Font)
		return {};
	return LastName();
}

Color UDXTextParser::GetColor()
{
	if (LastTag() < DeusExTextTags::TT_DefaultColor)
	{
		Color color;
		color.A = 255;
		color.R = 255;
		color.G = 127;
		color.B = 127;
		return color;
	}
	else if (LastTag() == DeusExTextTags::TT_RevertColor)
	{
		return DefaultColor();
	}
	else if (LastTag() > DeusExTextTags::TT_TextColor)
	{
		Color color;
		color.A = 255;
		color.R = 127;
		color.G = 127;
		color.B = 255;
		return color;
	}
	else
	{
		return LastColor();
	}
}

void UDXTextParser::GetEmailInfo(std::string& name, std::string& subject, std::string& from, std::string& to, std::string& cc)
{
	name = "";
	subject = "";
	from = "";
	to = "";
	cc = "";
	if (LastTag() == DeusExTextTags::TT_Email)
	{
		name = LastEmailName();
		subject = LastEmailSubject();
		from = LastEmailFrom();
		to = LastEmailTo();
		cc = LastEmailCC();
	}
}

void UDXTextParser::GetFileInfo(std::string& fileName, std::string& fileDescription)
{
	fileName = "";
	fileDescription = "";
	if (LastTag() == DeusExTextTags::TT_File)
	{
		fileName = LastFileName();
		fileDescription = LastFileDescription();
	}
}

void UDXTextParser::SetPlayerName(const std::string& newPlayerName)
{
	PlayerName() = newPlayerName;
}
