#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

#include "File.h"
#include "KeyValueFile.h"
#include "String.h"
#include "StringView.h"
#include "../debug/Debug.h"

void KeyValueFileSection::init(const std::string &name)
{
	this->name = name;
}

const std::string &KeyValueFileSection::getName() const
{
	return this->name;
}

int KeyValueFileSection::getPairCount() const
{
	return static_cast<int>(this->pairs.size());
}

const std::pair<std::string, std::string> &KeyValueFileSection::getPair(int index) const
{
	DebugAssertIndex(this->pairs, index);
	return this->pairs[index];
}

bool KeyValueFileSection::tryGetValue(const std::string &key, std::string_view &value) const
{
	const auto iter = std::find_if(this->pairs.begin(), this->pairs.end(),
		[&key](const std::pair<std::string, std::string> &pair)
	{
		return pair.first == key;
	});

	if (iter == this->pairs.end())
	{
		return false;
	}

	value = iter->second;
	return true;
}

bool KeyValueFileSection::tryGetBoolean(const std::string &key, bool &value) const
{
	std::string_view str;
	if (!this->tryGetValue(key, str))
	{
		return false;
	}
	else
	{
		if (StringView::caseInsensitiveEquals(str, "true"))
		{
			value = true;
			return true;
		}
		else if (StringView::caseInsensitiveEquals(str, "false"))
		{
			value = false;
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool KeyValueFileSection::tryGetInteger(const std::string &key, int &value) const
{
	std::string_view str;
	if (!this->tryGetValue(key, str))
	{
		return false;
	}
	else
	{
		try
		{
			size_t index = 0;
			value = std::stoi(std::string(str), &index);
			return index == str.size();
		}
		catch (std::exception)
		{
			return false;
		}
	}
}

bool KeyValueFileSection::tryGetDouble(const std::string &key, double &value) const
{
	std::string_view str;
	if (!this->tryGetValue(key, str))
	{
		return false;
	}
	else
	{
		try
		{
			size_t index = 0;
			value = std::stod(std::string(str), &index);
			return index == str.size();
		}
		catch (std::exception)
		{
			return false;
		}
	}
}

bool KeyValueFileSection::tryGetString(const std::string &key, std::string_view &value) const
{
	return this->tryGetValue(key, value);
}

void KeyValueFileSection::addPair(std::string &&key, std::string &&value)
{
	this->pairs.emplace_back(std::move(key), std::move(value));
}

void KeyValueFileSection::clear()
{
	this->pairs.clear();
}

bool KeyValueFile::init(const char *filename)
{
	if (!File::exists(filename))
	{
		DebugLogErrorFormat("Couldn't find key value file \"%s\".", filename);
		return false;
	}

	const std::string text = File::readAllText(filename);
	std::istringstream iss(text);

	// Check each line for a valid section or key-value pair. Start the line numbers at 1
	// since most users aren't programmers.
	std::string line;
	KeyValueFileSection *activeSection = nullptr;
	for (int lineNumber = 1; std::getline(iss, line); lineNumber++)
	{
		const bool isEmptyLine = line.empty() || ((line.size() == 1) && std::isspace(line.front()));
		if (isEmptyLine)
		{
			continue;
		}

		std::string_view filteredLine = line;
		filteredLine = StringView::trimFront(filteredLine);
		filteredLine = StringView::trimBack(filteredLine);

		const size_t commentIndex = filteredLine.find(KeyValueFile::COMMENT);
		if (commentIndex == 0)
		{
			// Comment covers the entire line.
			continue;
		}
		else if (commentIndex != std::string_view::npos)
		{
			filteredLine = filteredLine.substr(0, commentIndex);
		}

		if (filteredLine.empty())
		{
			continue;
		}
		else if (filteredLine.size() < 3)
		{
			// Not long enough to be a section or key-value pair.
			DebugLogErrorFormat("Syntax error \"%s\" (line %d) in %s.", std::string(filteredLine).c_str(), lineNumber, filename);
			return false;
		}

		const size_t sectionFrontIndex = filteredLine.find(KeyValueFile::SECTION_FRONT);
		const bool isSectionLine = sectionFrontIndex != std::string_view::npos;
		const bool isKeyValuePairLine = filteredLine.find(KeyValueFile::PAIR_SEPARATOR) != std::string::npos;
		if (isSectionLine)
		{
			const size_t sectionBackIndex = filteredLine.find(KeyValueFile::SECTION_BACK, sectionFrontIndex);
			const bool isSectionNameValid = (sectionBackIndex != std::string_view::npos) && (sectionBackIndex > (sectionFrontIndex + 1));
			if (!isSectionNameValid)
			{
				DebugLogErrorFormat("Invalid section \"%s\" (line %d) in %s.", std::string(filteredLine).c_str(), lineNumber, filename);
				return false;
			}

			std::string_view sectionName = filteredLine.substr(sectionFrontIndex + 1, sectionBackIndex - sectionFrontIndex - 1);
			sectionName = StringView::trimFront(StringView::trimBack(sectionName));

			const auto sectionIter = std::find_if(this->sections.begin(), this->sections.end(),
				[&sectionName](const KeyValueFileSection &section)
			{
				return section.getName() == sectionName;
			});

			if (sectionIter != this->sections.end())
			{
				DebugLogErrorFormat("Section \"%s\" (line %d) already defined in %s.", std::string(sectionName).c_str(), lineNumber, filename);
				return false;
			}

			KeyValueFileSection section;
			section.init(std::string(sectionName));
			this->sections.emplace_back(std::move(section));
			activeSection = &this->sections.back();
		}
		else if (isKeyValuePairLine)
		{
			std::string_view tokens[2];
			if (!StringView::splitExpected<2>(filteredLine, KeyValueFile::PAIR_SEPARATOR, tokens))
			{
				DebugLogErrorFormat("Invalid key-value pair \"%s\" (line %d) in %s.", std::string(filteredLine).c_str(), lineNumber, filename);
				return false;
			}

			const std::string_view key = StringView::trimFront(StringView::trimBack(tokens[0]));
			if (key.empty())
			{
				DebugLogErrorFormat("Empty key in \"%s\" (line %d) in %s.", std::string(filteredLine).c_str(), lineNumber, filename);
				return false;
			}

			if (activeSection == nullptr)
			{
				DebugLogWarningFormat("Ignoring \"%s\" (line %d), no active section in %s.", std::string(filteredLine).c_str(), lineNumber, filename);
				continue;
			}

			const std::string_view value = StringView::trimFront(tokens[1]);
			activeSection->addPair(std::string(key), std::string(value));
		}
		else
		{
			DebugLogErrorFormat("Invalid line \"%s\" (line %d) in %s.", line.c_str(), lineNumber, filename);
			return false;
		}
	}

	return true;
}

int KeyValueFile::getSectionCount() const
{
	return static_cast<int>(this->sections.size());
}

const KeyValueFileSection &KeyValueFile::getSection(int index) const
{
	DebugAssertIndex(this->sections, index);
	return this->sections[index];
}

const KeyValueFileSection *KeyValueFile::findSection(const std::string &name) const
{
	const auto iter = std::find_if(this->sections.begin(), this->sections.end(),
		[&name](const KeyValueFileSection &section)
	{
		return section.getName() == name;
	});

	if (iter == this->sections.end())
	{
		DebugLogWarningFormat("Couldn't find section \"%s\".", name.c_str());
		return nullptr;
	}

	return &(*iter);
}
