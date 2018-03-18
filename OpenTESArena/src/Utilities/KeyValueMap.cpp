#include <sstream>
#include <vector>

#include "KeyValueMap.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/String.h"

namespace
{
	const std::unordered_map<std::string, bool> KeyValueMapBooleans =
	{
		{ "True", true },
		{ "true", true },
		{ "False", false },
		{ "false", false }
	};
}

const char KeyValueMap::COMMENT = '#';

KeyValueMap::KeyValueMap(const std::string &filename)
	: filename(filename)
{
	std::string text = File::readAllText(filename);
	std::istringstream iss(text);

	// Check each line in "text" for valid key/value pairs.
	std::string line;
	while (std::getline(iss, line))
	{
		// Ignore comments and blank lines.
		if (line.length() == 0)
			continue;

		const char firstChar = line.at(0);
		if ((firstChar == KeyValueMap::COMMENT) ||
			(firstChar == '\r'))
		{
			continue;
		}

		// There must be two tokens: the key and value.
		std::vector<std::string> tokens = String::split(line, '=');
		DebugAssert(tokens.size() == 2, "Too many tokens at \"" + line + 
			"\" in " + filename + ".");

		// The strings could be trimmed of whitespace also, but I want the parser to 
		// be strict.
		const std::string &key = tokens.at(0);
		std::string value = String::trimLines(tokens.at(1));

		this->pairs.insert(std::make_pair(key, value));
	}
}

const std::string &KeyValueMap::getValue(const std::string &key) const
{
	const auto pairIter = this->pairs.find(key);
	DebugAssert(pairIter != this->pairs.end(), "Key \"" + key + 
		"\" not found in " + this->filename + ".");

	return pairIter->second;
}

bool KeyValueMap::getBoolean(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	const auto boolIter = KeyValueMapBooleans.find(value);
	DebugAssert(boolIter != KeyValueMapBooleans.end(), "\"" + value + "\" for \"" + 
		key + "\" in " + this->filename + " must be either true or false.");

	return boolIter->second;
}

int KeyValueMap::getInteger(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	return std::stoi(value);
}

double KeyValueMap::getDouble(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	return std::stod(value);
}

const std::string &KeyValueMap::getString(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	return value;
}

const std::unordered_map<std::string, std::string> &KeyValueMap::getAll() const
{
	return this->pairs;
}
