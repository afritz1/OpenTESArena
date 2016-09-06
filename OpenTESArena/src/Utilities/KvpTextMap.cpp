#include <sstream>
#include <vector>

#include "KvpTextMap.h"

#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/String.h"

namespace
{
	const std::unordered_map<std::string, bool> KvpTextMapBooleans =
	{
		{ "True", true },
		{ "False", false }
	};
}

const char KvpTextMap::COMMENT = '#';

KvpTextMap::KvpTextMap(const std::string &filename)
	: pairs()
{
	std::string text = File::toString(filename);
	std::istringstream iss(text);

	// Check each line in "text" for valid key/value pairs.
	std::string line;
	while (std::getline(iss, line))
	{
		// Ignore comments and blank lines.
		const char &firstChar = line.at(0);
		if ((firstChar == KvpTextMap::COMMENT) ||
			(firstChar == '\r') ||
			(firstChar == '\n'))
		{
			continue;
		}

		// There must be two tokens: the key and value.
		std::vector<std::string> tokens = String::split(line, '=');
		Debug::check(tokens.size() == 2, "KVP Text Map",
			"Too many tokens at \"" + line + "\".");

		// The strings could be trimmed of whitespace also, but I want the parser to 
		// be strict.
		const std::string &key = tokens.at(0);
		std::string value = String::trimLines(tokens.at(1));

		this->pairs.insert(std::make_pair(key, value));
	}
}

KvpTextMap::~KvpTextMap()
{

}

const std::string &KvpTextMap::getValue(const std::string &key) const
{
	const auto pairIter = this->pairs.find(key);
	Debug::check(pairIter != this->pairs.end(), "KVP Text Map",
		"Key \"" + key + "\" not found.");

	return pairIter->second;
}

bool KvpTextMap::getBoolean(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	Debug::check(KvpTextMapBooleans.find(value) != KvpTextMapBooleans.end(),
		"KVP Text Map", "Boolean \"" + value + "\" must be either True or False.");

	return KvpTextMapBooleans.at(value);
}

int KvpTextMap::getInteger(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	return std::stoi(value);
}

double KvpTextMap::getDouble(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	return std::stod(value);
}

std::string KvpTextMap::getString(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	return value;
}
