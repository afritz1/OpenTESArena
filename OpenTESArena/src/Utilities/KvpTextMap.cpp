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
		{ "true", true },
		{ "False", false },
		{ "false", false }
	};
}

const char KvpTextMap::COMMENT = '#';

KvpTextMap::KvpTextMap(const std::string &filename)
	: filename(filename)
{
	std::string text = File::toString(filename);
	std::istringstream iss(text);

	// Check each line in "text" for valid key/value pairs.
	std::string line;
	while (std::getline(iss, line))
	{
		// Ignore comments and blank lines.
		if (line.length() == 0)
			continue;

		const char firstChar = line.at(0);
		if ((firstChar == KvpTextMap::COMMENT) ||
			(firstChar == '\r'))
		{
			continue;
		}

		// There must be two tokens: the key and value.
		std::vector<std::string> tokens = String::split(line, '=');
		Debug::check(tokens.size() == 2, "KVP Text Map",
			"Too many tokens at \"" + line + "\" in " + filename + ".");

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
		"Key \"" + key + "\" not found in " + this->filename + ".");

	return pairIter->second;
}

bool KvpTextMap::getBoolean(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	const auto boolIter = KvpTextMapBooleans.find(value);
	Debug::check(boolIter != KvpTextMapBooleans.end(), "KVP Text Map",
		"\"" + value + "\" for \"" + key + "\" in " + this->filename +
		" must be either true or false.");

	return boolIter->second;
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

const std::string &KvpTextMap::getString(const std::string &key) const
{
	const std::string &value = this->getValue(key);
	return value;
}
