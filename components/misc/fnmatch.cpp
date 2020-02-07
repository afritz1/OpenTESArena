#include <regex>

#include "fnmatch.h"
#include "../debug/Debug.h"
#include "../utilities/String.h"

namespace
{
	// Converts fnmatch pattern to ECMAScript regex format.
	std::string PatternToECMA(const char *pattern)
	{
		std::string translated = pattern;
		translated = String::replace(translated, "\\", "\\\\");
		translated = String::replace(translated, ".", "\\.");
		translated = String::replace(translated, "?", "[^/]");
		translated = String::replace(translated, "*", "[^/]*");
		translated = String::replace(translated, "[!", "[^");
		return translated;
	}
}

// Only use this on the implementation side because it's not part of the official interface.
#undef FNM_SUCCESS
#define FNM_SUCCESS 0
#undef FNM_FAILURE
#define FNM_FAILURE (-1)

int fnmatch(const char *pattern, const char *str, int flags)
{
	if ((pattern == nullptr) || (str == nullptr))
	{
		DebugLogError("'pattern' or 'str' was null.");
		return FNM_FAILURE;
	}

	if (flags != 0)
	{
		DebugLogError("'flags' not supported in fnmatch() implementation.");
		return FNM_FAILURE;
	}

	static_cast<void>(flags);

	const std::string patternECMAString = PatternToECMA(pattern);
	const std::regex patternECMA(patternECMAString, std::regex_constants::icase);
	return std::regex_match(str, patternECMA) ? FNM_SUCCESS : FNM_NOMATCH;
}
