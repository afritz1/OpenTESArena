#include <regex>

#include "String.h"

std::vector<std::string> String::split(const std::string &line, char separator)
{
	std::vector<std::string> strings;

	// Add an empty string to start off. If the given line is empty, then a
	// vector with one empty string is returned.
	strings.push_back(std::string());

	for (const auto c : line)
	{
		if (c == separator)
		{
			// Start a new string.
			strings.push_back(std::string());
		}
		else
		{
			// Put the character on the current string.
			strings.at(strings.size() - 1).push_back(c);
		}
	}

	return strings;
}

std::vector<std::string> String::split(const std::string &line)
{
	return String::split(line, ' ');
}

std::string String::trim(const std::string &line)
{
	const char space = ' ';

	std::string trimmed;

	for (const auto c : line)
	{
		if (c != space)
		{
			trimmed.push_back(c);
		}
	}

	return trimmed;
}

std::string String::trimLines(const std::string &line)
{
	const char carriageReturn = '\r';
	const char newLine = '\n';

	std::string trimmed;

	for (const char c : line)
	{
		if ((c != carriageReturn) && (c != newLine))
		{
			trimmed.push_back(c);
		}
	}

	return trimmed;
}

std::string String::getExtension(const std::string &str)
{
	const size_t dotPos = str.rfind('.');
	const bool hasDot = (dotPos < str.length()) && (dotPos != std::string::npos);
	return hasDot ? std::string(str.begin() + dotPos, str.end()) : std::string();
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
	std::string newStr = std::regex_replace(str, std::regex("\\" + a), b);

	return newStr;
}
