#pragma once

#include <optional>
#include <string>

class UnrealURL
{
public:
	UnrealURL() = default;
	// Constructs an UnrealURL by passing the options from nextURL to the baseURL
	UnrealURL(const UnrealURL& baseURL, const UnrealURL& nextURL);
	// Constructs an UnrealURL from a given parse-able string.
	UnrealURL(std::string urlString);

	void AddOrReplaceOption(const std::string& newvalue);
	bool HasOption(std::string name) const;
	std::string GetOption(std::string name) const;
	std::string GetAddressURL() const;
	std::string GetOptions() const;
	std::string GetPortal() const;
	std::string ToString() const;
	bool Empty() const;
	void Clear();

	std::string Protocol = "unreal";
	std::string ProtocolDescription = "Unreal Protocol";
	std::string Name = "Player";
	std::string Map = "Index.unr";
	std::string Host;
	std::string Portal;
	std::string MapExt = "unr";
	std::string SaveExt = "usa";
	int Port = 7777;
	Array<std::string> Options;
};

// The slot a save-slot option or console argument names: an optional '-' followed by digits, within
// int range. Anything else (trailing text, an empty text, a sign alone) is not a number.
std::optional<int> ParseSaveSlot(const std::string& text);

// The text of the option that asks for a load: `load` (Unreal, UT), else `loadgame` (the Deus Ex
// scripts' spelling). Nothing when the URL carries neither; the text is empty for a bare `?load`.
std::optional<std::string> LoadOptionText(const UnrealURL& url);

// The name of the map a URL enters, as a save records it: the URL's map without a trailing
// ".<mapExtension>" (matched without regard to case). The teleport tag is not part of it.
std::string MapNameOf(const UnrealURL& url, const std::string& mapExtension);
