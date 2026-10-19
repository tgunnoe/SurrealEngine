#pragma once

#include <string>

std::string from_utf16(const std::wstring& str);
std::wstring to_utf16(const std::string& str);

// Narrows code units to the engine's byte strings: a unit below 256 is that
// byte, any other unit (a surrogate half included) becomes '?'. Unlike the
// platform pair above this is pure and available on every platform.
std::string narrow_utf16(const std::wstring& str);
