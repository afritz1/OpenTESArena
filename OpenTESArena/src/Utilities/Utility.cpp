#include "Utility.h"

std::vector<std::string> Utility::split(const std::string &line, char separator)
{
	auto strings = std::vector<std::string>();

	// Add an empty string to start off.
	strings.push_back(std::string());

	for (const auto &c : line)
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

std::vector<std::string> Utility::split(const std::string &line)
{
	return Utility::split(line, ' ');
}

std::string Utility::trim(const std::string &line)
{
	auto trimmed = std::string();

	const auto space = ' ';

	for (const auto &c : line)
	{
		if (c != space)
		{
			trimmed.push_back(c);
		}
	}

	return trimmed;
}

std::string Utility::trimLines(const std::string &line)
{
	auto trimmed = std::string();

	const auto carriageReturn = '\r';
	const auto newLine = '\n';

	for (const auto &c : line)
	{
		if ((c != carriageReturn) && (c != newLine))
		{
			trimmed.push_back(c);
		}
	}

	return trimmed;
}
