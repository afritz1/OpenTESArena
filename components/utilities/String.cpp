#include <cctype>
#include <cstring>

#include "String.h"

bool String::isNullOrEmpty(const char *str)
{
	return (str == nullptr) || (std::strlen(str) == 0);
}

bool String::caseInsensitiveEquals(const std::string &a, const std::string &b)
{
	if (a.size() != b.size())
	{
		return false;
	}

	for (size_t i = 0; i < a.size(); i++)
	{
		if (std::tolower(a[i]) != std::tolower(b[i]))
		{
			return false;
		}
	}

	return true;
}

std::vector<std::string> String::split(const std::string &str, char separator)
{
	std::vector<std::string> strings;

	// Add an empty string to start off. If the given string is empty, then a
	// vector with one empty string is returned.
	strings.push_back(std::string());

	for (const char c : str)
	{
		if (c == separator)
		{
			// Start a new string.
			strings.push_back(std::string());
		}
		else
		{
			// Put the character on the end of the current string.
			strings.back().push_back(c);
		}
	}

	return strings;
}

std::vector<std::string> String::split(const std::string &str)
{
	return String::split(str, String::SPACE);
}

std::string String::trim(const std::string &str)
{
	std::string trimmed;
	for (const char c : str)
	{
		if (!std::isspace(c))
		{
			trimmed.push_back(c);
		}
	}

	return trimmed;
}

std::string String::trimFront(const std::string &str)
{
	std::string trimmed(str);	
	while ((trimmed.size() > 0) && std::isspace(trimmed.front()))
	{
		trimmed.erase(trimmed.begin());
	}

	return trimmed;
}

void String::trimFrontInPlace(std::string &str)
{
	while ((str.size() > 0) && std::isspace(str.front()))
	{
		str.erase(str.begin());
	}
}

std::string String::trimBack(const std::string &str)
{
	std::string trimmed(str);
	while ((trimmed.size() > 0) && std::isspace(trimmed.back()))
	{
		trimmed.pop_back();
	}

	return trimmed;
}

void String::trimBackInPlace(std::string &str)
{
	while ((str.size() > 0) && std::isspace(str.back()))
	{
		str.pop_back();
	}
}

std::string String::trimLines(const std::string &str)
{
	std::string trimmed;
	for (const char c : str)
	{
		if ((c != String::CARRIAGE_RETURN) && (c != String::NEWLINE))
		{
			trimmed.push_back(c);
		}
	}

	return trimmed;
}

std::string String::trimExtra(const std::string &str)
{
	std::string trimmed;
	char prev = -1;

	for (const char c : str)
	{
		if (!std::isspace(c) || !std::isspace(prev))
		{
			trimmed += c;
			prev = c;
		}
	}

	return trimmed;
}

std::string String::distributeNewlines(const std::string &str, int charLimit)
{
	constexpr int NO_SPACE = -1;
	std::string newStr = str;
	int mostRecentSpace = NO_SPACE;
	int currentLineLength = 0;

	for (int i = 0; i < newStr.size(); i++, currentLineLength++)
	{
		if (newStr[i] == String::SPACE)
		{
			mostRecentSpace = i;
		}

		// Only try to add a newline if it's not the last character.
		if ((currentLineLength == charLimit) && (i != static_cast<int>(newStr.size() - 1)))
		{
			if (mostRecentSpace == NO_SPACE)
			{
				newStr.insert(newStr.begin() + i + 1, String::NEWLINE);
			}
			else
			{
				newStr[mostRecentSpace] = String::NEWLINE;
				mostRecentSpace = NO_SPACE;
			}
			
			currentLineLength = 0;
		}
	}

	return newStr;
}

std::string String::getExtension(const std::string &str)
{
	const size_t dotPos = str.rfind(String::FILE_EXTENSION_SEPARATOR);
	const bool hasDot = (dotPos < str.length()) && (dotPos != std::string::npos);
	return hasDot ? std::string(str.begin() + dotPos + 1, str.end()) : std::string();
}

std::string String::addTrailingSlashIfMissing(const std::string &str)
{
	if (str.empty())
	{
		return "/";
	}
	else if ((str.back() != '/') && (str.back() != '\\'))
	{
		return str + '/';
	}
	else
	{
		return str;
	}
}

std::string String::replace(const std::string &str, char a, char b)
{
	std::string newStr(str);
	for (char &c : newStr)
	{
		if (c == a)
		{
			c = b;
		}
	}

	return newStr;
}

std::string String::replace(const std::string &str, const std::string &a, 
	const std::string &b)
{
	// Replace all instances of "a" with "b".
	std::string newStr(str);
	for (size_t index = newStr.find(a); index != std::string::npos; 
		index = newStr.find(a, index))
	{
		newStr.replace(index, a.size(), b);
		index += b.size();
	}

	return newStr;
}

std::string String::toUppercase(const std::string &str)
{
	std::string newStr(str);
	for (char &c : newStr)
	{
		c = std::toupper(c);
	}

	return newStr;
}

std::string String::toLowercase(const std::string &str)
{
	std::string newStr(str);
	for (char &c : newStr)
	{
		c = std::tolower(c);
	}

	return newStr;
}

bool String::tryCopy(const char *src, char *dst, size_t dstSize)
{
	const int status = std::snprintf(dst, dstSize, "%s", src);
	return (status >= 0) && (status < dstSize);
}
