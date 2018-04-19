#include <cassert>
#include <sstream>
#include <vector>

#include "KeyValueMap.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/String.h"

namespace
{
	const std::unordered_map<std::string, bool> BooleanStrings =
	{
		{ "True", true },
		{ "true", true },
		{ "False", false },
		{ "false", false }
	};
}

const char KeyValueMap::COMMENT = '#';
const char KeyValueMap::PAIR_SEPARATOR = '=';
const char KeyValueMap::SECTION_FRONT = '[';
const char KeyValueMap::SECTION_BACK = ']';

KeyValueMap::KeyValueMap(const std::string &filename)
	: filename(filename)
{
	const std::string text = File::readAllText(filename);
	std::istringstream iss(text);

	// Check each line for a valid section or key-value pair. Start the line numbers at 1
	// since most users aren't programmers.
	std::string line;
	std::string section;
	SectionMap *activeSectionMap = nullptr;
	for (int lineNumber = 1; std::getline(iss, line); lineNumber++)
	{
		// Get a filtered version of the current line so it can be parsed. If the filtered
		// string is empty, then skip to the next line.
		// - To do: std::string_view would be useful here. The skip/remove/comment code
		//   combined could return the usable portion of the line.
		const std::string filteredLine = [&line]()
		{
			std::string str = line;

			// Skip empty strings.
			if ((str.size() == 0) || ((str.size() == 1) && (str.front() == '\r')))
			{
				return std::string();
			}

			// Remove carriage return at the end (if any).
			if (str.back() == '\r')
			{
				str.pop_back();
			}

			// Extract left-most comment (if any).
			const size_t commentIndex = str.find(KeyValueMap::COMMENT);
			if (commentIndex == 0)
			{
				// Comment covers the entire line.
				return std::string();
			}
			else if (commentIndex != std::string::npos)
			{
				// Comment is somewhere in the line, so only work with the left substring.
				str = str.substr(0, commentIndex);
			}

			// Trim leading and trailing whitespace (i.e., in case the key has whitespace
			// before it, or a comment had whitespace before it).
			return String::trimFront(String::trimBack(str));
		}();

		if (filteredLine.empty())
		{
			// It became an empty string after whitespace was removed.
			continue;
		}
		else if (filteredLine.size() < 3)
		{
			// Not long enough to be a section or key-value pair.
			DebugCrash("Syntax error \"" + filteredLine + "\" (line " +
				std::to_string(lineNumber) + ") in " + filename + ".");
		}

		// See if it's a section line or key-value pair line.
		const size_t sectionFrontIndex = filteredLine.find(KeyValueMap::SECTION_FRONT);
		if (sectionFrontIndex != std::string::npos)
		{
			// Section line. There must be a closing character with enough space between it
			// and the front character for at least one section character.
			const size_t sectionBackIndex = filteredLine.find(
				KeyValueMap::SECTION_BACK, sectionFrontIndex);

			if ((sectionBackIndex != std::string::npos) &&
				(sectionBackIndex > (sectionFrontIndex + 1)))
			{
				// Get the string that's between the section characters and trim any
				// leading or trailing whitespace.
				std::string sectionName = filteredLine.substr(
					sectionFrontIndex + 1, sectionBackIndex - sectionFrontIndex - 1);
				sectionName = String::trimFront(String::trimBack(sectionName));

				auto sectionIter = this->sectionMaps.find(sectionName);

				// If the section is new, add it to the section maps.
				if (sectionIter == this->sectionMaps.end())
				{
					sectionIter = this->sectionMaps.insert(std::make_pair(
						std::move(sectionName), SectionMap())).first;
					activeSectionMap = &sectionIter->second;
				}
				else
				{
					DebugCrash("Section \"" + sectionName + "\" (line " +
						std::to_string(lineNumber) + ") already defined in " + filename + ".");
				}
			}
			else
			{
				DebugCrash("Invalid section \"" + filteredLine + "\" (line " +
					std::to_string(lineNumber) + ") in " + filename + ".");
			}
		}
		else if (filteredLine.find(KeyValueMap::PAIR_SEPARATOR) != std::string::npos)
		{
			// Key-value pair line. There must be two tokens: key and value.
			const std::vector<std::string> tokens = String::split(
				filteredLine, KeyValueMap::PAIR_SEPARATOR);

			if (tokens.size() != 2)
			{
				DebugCrash("Invalid pair \"" + filteredLine + "\" (line " +
					std::to_string(lineNumber) + ") in " + filename + ".");
			}

			// Trim trailing whitespace from the key and leading whitespace from the value.
			std::string key = String::trimBack(tokens.front());
			std::string value = String::trimFront(tokens.back());

			if (key.size() == 0)
			{
				DebugCrash("Empty key in \"" + filteredLine + "\" (line " +
					std::to_string(lineNumber) + ") in " + filename + ".");
			}

			// Add the key-value pair to the active section map.
			if (activeSectionMap != nullptr)
			{
				activeSectionMap->insert(std::make_pair(std::move(key), std::move(value)));
			}
			else
			{
				// If no active section map, print a warning and ignore the current pair.
				// All key-value pairs must be in a section.
				DebugWarning("Ignoring \"" + filteredLine + "\" (line " +
					std::to_string(lineNumber) + "), no active section in " + filename);
			}
		}
		else
		{
			// Filtered line is not a section or key-value pair.
			DebugCrash("Invalid line \"" + line + "\" (line " +
				std::to_string(lineNumber) + ") in " + filename + ".");
		}
	}
}

const std::string &KeyValueMap::getValue(const std::string &section, const std::string &key) const
{
	const auto sectionIter = this->sectionMaps.find(section);

	if (sectionIter == this->sectionMaps.end())
	{
		throw std::runtime_error("Section \"" + section +
			"\" not found in " + this->filename + ".");
	}
	else
	{
		const SectionMap &sectionMap = sectionIter->second;
		const auto keyIter = sectionMap.find(key);
		if (keyIter == sectionMap.end())
		{
			throw std::runtime_error("Key \"" + key + "\" not found in " +
				KeyValueMap::SECTION_FRONT + section + KeyValueMap::SECTION_BACK +
				" in " + this->filename + ".");
		}
		else
		{
			return keyIter->second;
		}
	}
}

bool KeyValueMap::getBoolean(const std::string &section, const std::string &key) const
{
	const std::string &value = this->getValue(section, key);
	const auto iter = BooleanStrings.find(value);
	DebugAssert(iter != BooleanStrings.end(), "\"" + key + "\" value \"" +
		value + "\" in " + this->filename + " must be true or false.");

	return iter->second;
}

int KeyValueMap::getInteger(const std::string &section, const std::string &key) const
{
	const std::string &value = this->getValue(section, key);
	return std::stoi(value);
}

double KeyValueMap::getDouble(const std::string &section, const std::string &key) const
{
	const std::string &value = this->getValue(section, key);
	return std::stod(value);
}

const std::string &KeyValueMap::getString(const std::string &section, const std::string &key) const
{
	const std::string &value = this->getValue(section, key);
	return value;
}

const std::unordered_map<std::string, KeyValueMap::SectionMap> &KeyValueMap::getAll() const
{
	return this->sectionMaps;
}
