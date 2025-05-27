#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

#include "File.h"
#include "KeyValueFile.h"
#include "String.h"
#include "StringView.h"
#include "../debug/Debug.h"

namespace
{
	bool KeyValuePairComparer(const std::pair<std::string, std::string> &a, const std::pair<std::string, std::string> &b)
	{
		return a.first < b.first;
	}
}

void KeyValueFileSection::init(std::string &&name)
{
	this->name = std::move(name);
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
	const std::pair<std::string, std::string> searchPair(key, std::string());
	const auto iter = std::lower_bound(this->pairs.begin(), this->pairs.end(), searchPair, KeyValuePairComparer);

	if ((iter != this->pairs.end()) && (iter->first == key))
	{
		value = iter->second;
		return true;
	}
	else
	{
		return false;
	}
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

void KeyValueFileSection::add(std::string &&key, std::string &&value)
{
	this->pairs.emplace_back(std::move(key), std::move(value));
	std::sort(this->pairs.begin(), this->pairs.end(), KeyValuePairComparer);
}

void KeyValueFileSection::clear()
{
	this->pairs.clear();
}

bool KeyValueFile::init(const char *filename)
{
	if (!File::exists(filename))
	{
		DebugLogError("Could not find \"" + std::string(filename) + "\".");
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
		// Get a filtered version of the current line so it can be parsed. If the filtered
		// string is empty, then skip to the next line.
		const std::string_view filteredLine = [&line]()
		{
			std::string_view str = line;

			// Skip empty strings.
			if ((str.size() == 0) || ((str.size() == 1) && std::isspace(str.front())))
			{
				return std::string_view();
			}

			str = StringView::trimFront(str);
			str = StringView::trimBack(str);

			// Extract left-most comment (if any).
			const size_t commentIndex = str.find(KeyValueFile::COMMENT);
			if (commentIndex == 0)
			{
				// Comment covers the entire line.
				return std::string_view();
			}
			else if (commentIndex != std::string_view::npos)
			{
				// Comment is somewhere in the line, so only work with the left substring.
				str = str.substr(0, commentIndex);

				// Trim any whitespace between the end of the value and the beginning of the comment.
				str = StringView::trimBack(str);
			}

			return str;
		}();

		if (filteredLine.empty())
		{
			// It became an empty string after whitespace was removed.
			continue;
		}
		else if (filteredLine.size() < 3)
		{
			// Not long enough to be a section or key-value pair.
			DebugLogError("Syntax error \"" + std::string(filteredLine) + "\" (line " +
				std::to_string(lineNumber) + ") in " + filename + ".");
			return false;
		}

		// See if it's a section line or key-value pair line.
		const size_t sectionFrontIndex = filteredLine.find(KeyValueFile::SECTION_FRONT);
		if (sectionFrontIndex != std::string_view::npos)
		{
			// Section line. There must be a closing character with enough space between it
			// and the front character for at least one section character.
			const size_t sectionBackIndex = filteredLine.find(
				KeyValueFile::SECTION_BACK, sectionFrontIndex);

			if ((sectionBackIndex != std::string_view::npos) &&
				(sectionBackIndex > (sectionFrontIndex + 1)))
			{
				// Get the string that's between the section characters and trim any
				// leading or trailing whitespace.
				std::string_view sectionName = filteredLine.substr(
					sectionFrontIndex + 1, sectionBackIndex - sectionFrontIndex - 1);
				sectionName = StringView::trimFront(StringView::trimBack(sectionName));

				const auto sectionIter = std::find_if(this->sections.begin(), this->sections.end(),
					[&sectionName](const KeyValueFileSection &section)
				{
					return section.getName() == sectionName;
				});

				// If the section is new, add it to the section maps.
				if (sectionIter == this->sections.end())
				{
					KeyValueFileSection section;
					section.init(std::string(sectionName));
					this->sections.push_back(std::move(section));
					activeSection = &this->sections.back();
				}
				else
				{
					DebugLogError("Section \"" + std::string(sectionName) + "\" (line " +
						std::to_string(lineNumber) + ") already defined in " + filename + ".");
					return false;
				}
			}
			else
			{
				DebugLogError("Invalid section \"" + std::string(filteredLine) + "\" (line " +
					std::to_string(lineNumber) + ") in " + filename + ".");
				return false;
			}
		}
		else if (filteredLine.find(KeyValueFile::PAIR_SEPARATOR) != std::string::npos)
		{
			// Key-value pair line. There must be two tokens: key and value.
			std::array<std::string_view, 2> tokens;
			if (!StringView::splitExpected<2>(filteredLine, KeyValueFile::PAIR_SEPARATOR, tokens))
			{
				DebugLogError("Invalid pair \"" + std::string(filteredLine) + "\" (line " +
					std::to_string(lineNumber) + ") in " + filename + ".");
				return false;
			}

			// Trim whitespace from the key and leading whitespace from the value.
			const std::string_view key = StringView::trimFront(StringView::trimBack(tokens[0]));
			const std::string_view value = StringView::trimFront(tokens[1]);

			if (key.size() == 0)
			{
				DebugLogError("Empty key in \"" + std::string(filteredLine) + "\" (line " +
					std::to_string(lineNumber) + ") in " + filename + ".");
				return false;
			}

			// Add the key-value pair to the active section map.
			if (activeSection != nullptr)
			{
				activeSection->add(std::string(key), std::string(value));
			}
			else
			{
				// If no active section map, print a warning and ignore the current pair.
				// All key-value pairs must be in a section.
				DebugLogWarning("Ignoring \"" + std::string(filteredLine) + "\" (line " +
					std::to_string(lineNumber) + "), no active section in " + filename);
			}
		}
		else
		{
			// Filtered line is not a section or key-value pair.
			DebugLogError("Invalid line \"" + line + "\" (line " +
				std::to_string(lineNumber) + ") in " + filename + ".");
			return false;
		}
	}

	// Sort for binary search.
	std::sort(this->sections.begin(), this->sections.end(),
		[](const KeyValueFileSection &sectionA, const KeyValueFileSection &sectionB)
	{
		return sectionA.getName() < sectionB.getName();
	});

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
	const auto iter = std::lower_bound(this->sections.begin(), this->sections.end(), name,
		[](const KeyValueFileSection &section, const std::string &str)
	{
		return section.getName() < str;
	});

	if ((iter == this->sections.end()) || (iter->getName() != name))
	{
		DebugLogWarning("Couldn't find section \"" + name + "\".");
		return nullptr;
	}

	return &(*iter);
}
