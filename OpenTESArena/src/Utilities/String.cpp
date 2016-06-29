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
	const auto space = ' ';

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
	const auto carriageReturn = '\r';
	const auto newLine = '\n';

	std::string trimmed;

	for (const auto c : line)
	{
		if ((c != carriageReturn) && (c != newLine))
		{
			trimmed.push_back(c);
		}
	}

	return trimmed;
}
