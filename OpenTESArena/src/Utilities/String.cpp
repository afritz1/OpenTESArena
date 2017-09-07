#include <cctype>

#include "String.h"

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
			strings.at(strings.size() - 1).push_back(c);
		}
	}

	return strings;
}

std::vector<std::string> String::split(const std::string &str)
{
	return String::split(str, ' ');
}

std::string String::trim(const std::string &str)
{
	const char space = ' ';

	std::string trimmed;

	for (const char c : str)
	{
		if (c != space)
		{
			trimmed.push_back(c);
		}
	}

	return trimmed;
}

std::string String::trimFront(const std::string &str)
{
	const char space = ' ';

	std::string trimmed(str);
	
	while (trimmed.front() == space)
	{
		trimmed.erase(trimmed.begin());
	}

	return trimmed;
}

std::string String::trimBack(const std::string &str)
{
	const char space = ' ';

	std::string trimmed(str);

	while (trimmed.back() == space)
	{
		trimmed.pop_back();
	}

	return trimmed;
}

std::string String::trimLines(const std::string &str)
{
	const char carriageReturn = '\r';
	const char newLine = '\n';

	std::string trimmed;

	for (const char c : str)
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

	for (auto &c : newStr)
	{
		c = std::toupper(c);
	}

	return newStr;
}

std::string String::toLowercase(const std::string &str)
{
	std::string newStr(str);

	for (auto &c : newStr)
	{
		c = std::tolower(c);
	}

	return newStr;
}
