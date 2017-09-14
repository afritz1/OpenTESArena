#include <algorithm>
#include <cctype>
#include <sstream>

#include "TextAssets.h"

#include "ExeStrings.h"
#include "ExeUnpacker.h"
#include "../Entities/CharacterClassCategoryName.h"
#include "../Utilities/Debug.h"
#include "../Utilities/Platform.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

const std::string TextAssets::AExeKeyValuesMapPath = "data/text/aExeStrings.txt";

TextAssets::TextAssets()
{
	// Decompress A.EXE and place it in a string for later use.
	const ExeUnpacker floppyExe("A.EXE");
	this->aExe = floppyExe.getText();

	// Generate a map of interesting strings from the text of A.EXE.
	this->aExeStrings = std::unique_ptr<ExeStrings>(new ExeStrings(
		this->aExe, Platform::getBasePath() + TextAssets::AExeKeyValuesMapPath));

	// Read in TEMPLATE.DAT, using "#..." as keys and the text as values.
	this->parseTemplateDat();

	// Read in QUESTION.TXT and create character question objects.
	this->parseQuestionTxt();

	// Read in DUNGEON.TXT and pair each dungeon name with its description.
	this->parseDungeonTxt();
}

TextAssets::~TextAssets()
{

}

void TextAssets::parseTemplateDat()
{
	const std::string filename = "TEMPLATE.DAT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	// Read TEMPLATE.DAT into a string.
	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<char> bytes(fileSize);
	stream->read(bytes.data(), fileSize);

	const std::string text(bytes.data(), fileSize);

	// Step line by line through the text, inserting keys and values into the map.
	std::istringstream iss(text);
	std::string line, key, value;

	while (std::getline(iss, line))
	{
		const char poundSign = '#';
		if (line.at(0) == poundSign)
		{
			// Add the previous key/value pair into the map. There are multiple copies of 
			// some texts in TEMPLATE.DAT, so it's important to skip existing ones.
			if (this->templateDat.find(key) == this->templateDat.end())
			{
				// Clean up the text first so the caller has to do less.
				value = String::replace(value, '\r', '\n');

				while ((value.size() > 0) && (value.at(value.size() - 1) == '\n'))
				{
					value.pop_back();
				}

				// Remove the annoying ampersand at the end of most texts.
				if ((value.size() > 0) && (value.at(value.size() - 1) == '&'))
				{
					value.pop_back();
				}

				this->templateDat.insert(std::make_pair(key, value));
			}

			// Reset the key and value for the next paragraph(s) of text.
			key = String::trim(String::trimLines(line));
			value = "";
		}
		else
		{
			// Add the current line of text onto the value.
			value.append(line);
		}
	}

	// Remove the one empty string added at the start (when key is "").
	this->templateDat.erase("");
}

void TextAssets::parseQuestionTxt()
{
	const std::string filename = "QUESTION.TXT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	// Read QUESTION.TXT into a string.
	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<char> bytes(fileSize);
	stream->read(bytes.data(), fileSize);

	const std::string text(bytes.data(), fileSize);

	// Lambda for adding a new question to the questions list.
	auto addQuestion = [this](const std::string &description,
		const std::string &a, const std::string &b, const std::string &c)
	{
		// Lambda for determining which choices point to which class categories.
		auto getCategory = [](const std::string &choice)
		{
			const char mageChar = 'l'; // Logical?
			const char thiefChar = 'c'; // Clever?
			const char warriorChar = 'v'; // Violent?
			const char categoryChar = choice.at(choice.find("(5") + 2);

			if (categoryChar == mageChar)
			{
				return CharacterClassCategoryName::Mage;
			}
			else if (categoryChar == thiefChar)
			{
				return CharacterClassCategoryName::Thief;
			}
			else if (categoryChar == warriorChar)
			{
				return CharacterClassCategoryName::Warrior;
			}
			else
			{
				throw std::runtime_error("Bad QUESTION.TXT class category.");
			}
		};

		this->questionTxt.push_back(CharacterQuestion(description,
			std::make_pair(a, getCategory(a)),
			std::make_pair(b, getCategory(b)),
			std::make_pair(c, getCategory(c))));
	};

	// Step line by line through the text, creating question objects.
	std::istringstream iss(text);
	std::string line, description, a, b, c;

	enum class Mode { Description, A, B, C };
	Mode mode = Mode::Description;

	while (std::getline(iss, line))
	{
		const char ch = line.at(0);

		if (std::isalpha(ch))
		{
			// See if it's 'a', 'b', or 'c', and switch to that mode.
			if (ch == 'a')
			{
				mode = Mode::A;
			}
			else if (ch == 'b')
			{
				mode = Mode::B;
			}
			else if (ch == 'c')
			{
				mode = Mode::C;
			}
		}
		else if (std::isdigit(ch))
		{
			// If previous data was read, push it onto the questions list.
			if (mode != Mode::Description)
			{
				addQuestion(description, a, b, c);

				// Start over each string for the next question object.
				description.clear();
				a.clear();
				b.clear();
				c.clear();
			}

			mode = Mode::Description;
		}

		// Add back the newline that was removed by std::getline().
		line += '\n';

		// Append the line onto the current string depending on the mode.
		if (mode == Mode::Description)
		{
			description += line;
		}
		else if (mode == Mode::A)
		{
			a += line;
		}
		else if (mode == Mode::B)
		{
			b += line;
		}
		else if (mode == Mode::C)
		{
			c += line;
		}
	}

	// Add the last question object (#40) with the data collected by the last line
	// in the file (it's skipped in the loop).
	addQuestion(description, a, b, c);
}

void TextAssets::parseDungeonTxt()
{
	const std::string filename = "DUNGEON.TXT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<char> bytes(fileSize);
	stream->read(bytes.data(), fileSize);

	const std::string text(bytes.data(), fileSize);

	// Step line by line through the text, inserting data into the dungeon list.
	std::istringstream iss(text);
	std::string line, title, description;

	while (std::getline(iss, line))
	{
		const char poundSign = '#';
		if (line.at(0) == poundSign)
		{
			// Remove the newline from the end of the description.
			if (description.at(description.size() - 1) == '\n')
			{
				description.pop_back();
			}

			// Put the collected data into the list and restart the title and description.
			this->dungeonTxt.push_back(std::make_pair(title, description));
			title.clear();
			description.clear();
		}
		else if (title.empty())
		{
			// It's either the first line in the file or it's right after a '#', so it's 
			// a dungeon name.
			title = line;

			// Remove the carriage return if it exists.
			const size_t titleCarriageReturn = title.find('\r');
			if (titleCarriageReturn != std::string::npos)
			{
				title = title.replace(titleCarriageReturn, 1, "");
			}
		}
		else
		{
			// It's part of a dungeon description. Append it to the current description.
			description += line;

			// Replace the carriage return with a newline.
			const size_t descriptionCarriageReturn = description.find('\r');
			if (descriptionCarriageReturn != std::string::npos)
			{
				description = description.replace(descriptionCarriageReturn, 1, "\n");
			}
		}
	}
}

const ExeStrings &TextAssets::getAExeStrings() const
{
	return *this->aExeStrings.get();
}

const std::string &TextAssets::getTemplateDatText(const std::string &key)
{
	const auto iter = this->templateDat.find(key);
	DebugAssert(iter != this->templateDat.end(), "TEMPLATE.DAT key \"" + 
		key + "\" not found.");

	const std::string &value = iter->second;
	return value;
}

const std::vector<CharacterQuestion> &TextAssets::getQuestionTxtQuestions() const
{
	return this->questionTxt;
}

const std::vector<std::pair<std::string, std::string>> &TextAssets::getDungeonTxtDungeons() const
{
	return this->dungeonTxt;
}
