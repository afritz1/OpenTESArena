#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>

#include "TextAssetLibrary.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/String.h"
#include "components/vfs/manager.hpp"

namespace
{
	enum class NameRuleType
	{
		Index, // Points into chunk lists.
		String, // Pre-defined string.
		IndexChance, // Points into chunk lists, with a chance to not be used.
		IndexStringChance, // Points into chunk lists, w/ string and chance.
	};

	struct NameRuleIndexChance
	{
		int index, chance;
	};

	struct NameRuleIndexStringChance
	{
		int index;
		std::array<char, 4> str;
		int chance;
	};

	// Discriminated union for name composition rules used with NAMECHNK.DAT.
	// Each rule is either:
	// - Index
	// - Pre-defined string
	// - Index with chance
	// - Index and string with chance
	struct NameRule
	{
		NameRuleType type;

		union
		{
			int index;
			std::array<char, 4> str;
			NameRuleIndexChance indexChance;
			NameRuleIndexStringChance indexStringChance;
		};

		NameRule(int index)
		{
			this->type = NameRuleType::Index;
			this->index = index;
		}

		NameRule(const std::string &str)
		{
			this->type = NameRuleType::String;
			this->str.fill('\0');
			const size_t charCount = std::min(str.size(), this->str.size());
			std::copy(str.begin(), str.begin() + charCount, this->str.begin());
		}

		NameRule(int index, int chance)
		{
			this->type = NameRuleType::IndexChance;
			this->indexChance.index = index;
			this->indexChance.chance = chance;
		}

		NameRule(int index, const std::string &str, int chance)
		{
			this->type = NameRuleType::IndexStringChance;
			this->indexStringChance.index = index;

			this->indexStringChance.str.fill('\0');
			const size_t charCount = std::min(str.size(), this->indexStringChance.str.size());
			std::copy(str.begin(), str.begin() + charCount, this->indexStringChance.str.begin());

			this->indexStringChance.chance = chance;
		}
	};

	// Rules for how to access NAMECHNK.DAT lists for name creation (with associated
	// chances, if any).
	const std::array<std::vector<NameRule>, 48> NameRules =
	{
		{
			// Race 0.
			{ { 0 }, { 1 }, { " " }, { 4 }, { 5 } },
			{ { 2 }, { 3 }, { " " }, { 4 }, { 5 } },

			// Race 1.
			{ { 6 }, { 7 }, { 8 }, { 9, 75 } },
			{ { 6 }, { 7 }, { 8 }, { 9, 75 }, { 10 } },

			// Race 2.
			{ { 11 }, { 12 }, { " " }, { 15 }, { 16 }, { "sen" } },
			{ { 13 }, { 14 }, { " " }, { 15 }, { 16 }, { "sen" } },

			// Race 3.
			{ { 17 }, { 18 }, { " " }, { 21 }, { 22 } },
			{ { 19 }, { 20 }, { " " }, { 21 }, { 22 } },

			// Race 4.
			{ { 23 }, { 24 }, { " " }, { 27 }, { 28 } },
			{ { 25 }, { 26 }, { " " }, { 27 }, { 28 } },

			// Race 5.
			{ { 29 }, { 30 }, { " " }, { 33 }, { 34 } },
			{ { 31 }, { 32 }, { " " }, { 33 }, { 34 } },

			// Race 6.
			{ { 35 }, { 36 }, { " " }, { 39 }, { 40 } },
			{ { 37 }, { 38 }, { " " }, { 39 }, { 40 } },

			// Race 7.
			{ { 41 }, { 42 }, { " " }, { 45 }, { 46 } },
			{ { 43 }, { 44 }, { " " }, { 45 }, { 46 } },

			// Race 8.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 9.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 10.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 11.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 12.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 13.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 14.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 15.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 16.
			{ { 47 }, { 48, 75 }, { 49 } },
			{ { 47 }, { 48, 75 }, { 49 } },

			// Race 17.
			{ { 50 }, { 51, 75 }, { 52 } },
			{ { 50 }, { 51, 75 }, { 52 } },

			// Race 18.
			{ { 50 }, { 51, 75 }, { 52 } },
			{ { 50 }, { 51, 75 }, { 52 } },

			// Race 19.
			{ { 50 }, { 51, 75 }, { 52 } },
			{ { 50 }, { 51, 75 }, { 52 } },

			// Race 20.
			{ { 50 }, { 51, 75 }, { 52 } },
			{ { 50 }, { 51, 75 }, { 52 } },

			// Race 21.
			{ { 50 }, { 52 }, { 53 } },
			{ { 50 }, { 52 }, { 53 } },

			// Race 22.
			{ { 54, " ", 25 }, { 55 }, { 56 }, { 57 } },
			{ { 54, " ", 25 }, { 55 }, { 56 }, { 57 } },

			// Race 23.
			{ { 55 }, { 56 }, { 57 } },
			{ { 55 }, { 56 }, { 57 } }
		}
	};
}

const ArenaTemplateDatEntry &ArenaTemplateDat::getEntry(int key) const
{
	// Use first vector for non-tileset entry requests.
	DebugAssertMsg(!this->entryLists.empty(), "Missing TEMPLATE.DAT entry lists.");
	const BufferView<const ArenaTemplateDatEntry> entryList = this->entryLists[0];

	const auto iter = std::lower_bound(entryList.begin(), entryList.end(), key,
		[](const ArenaTemplateDatEntry &a, int key)
	{
		return a.key < key;
	});

	if (iter == entryList.end())
	{
		DebugCrashFormat("No TEMPLATE.DAT entry for \"%d\".", key);
	}

	return *iter;
}

const ArenaTemplateDatEntry &ArenaTemplateDat::getEntry(int key, char letter) const
{
	// Use first vector for non-tileset entry requests.
	DebugAssertMsg(!this->entryLists.empty(), "Missing TEMPLATE.DAT entry lists.");
	const BufferView<const ArenaTemplateDatEntry> entryList = this->entryLists[0];

	// The requested entry has a letter in its key, so need to find the range of
	// equal values for 'key' via binary search.
	const auto lowerIter = std::lower_bound(entryList.begin(), entryList.end(), key,
		[](const ArenaTemplateDatEntry &a, int key)
	{
		return a.key < key;
	});

	const auto upperIter = std::upper_bound(lowerIter, entryList.end(), key,
		[](int key, const ArenaTemplateDatEntry &b)
	{
		return key < b.key;
	});

	// Find 'letter' in the range of equal key values.
	const auto letterIter = std::lower_bound(lowerIter, upperIter, letter,
		[](const ArenaTemplateDatEntry &a, char letter)
	{
		return a.letter < letter;
	});

	if (letterIter == upperIter)
	{
		DebugCrashFormat("No TEMPLATE.DAT entry for \"%d, %d\".", key, letter);
	}

	return *letterIter;
}

const ArenaTemplateDatEntry &ArenaTemplateDat::getTilesetEntry(int tileset, int key, char letter) const
{
	DebugAssertIndex(this->entryLists, tileset);
	const BufferView<const ArenaTemplateDatEntry> entryList = this->entryLists[tileset];

	// Do binary search in the tileset vector to find the equal range for 'key'.
	const auto lowerIter = std::lower_bound(entryList.begin(), entryList.end(), key,
		[](const ArenaTemplateDatEntry &a, int key)
	{
		return a.key < key;
	});

	const auto upperIter = std::upper_bound(lowerIter, entryList.end(), key,
		[](int key, const ArenaTemplateDatEntry &b)
	{
		return key < b.key;
	});

	// Find 'letter' in the range of equal key values.
	const auto letterIter = std::lower_bound(lowerIter, upperIter, letter,
		[](const ArenaTemplateDatEntry &a, char letter)
	{
		return a.letter < letter;
	});

	if (letterIter == upperIter)
	{
		DebugCrashFormat("No TEMPLATE.DAT entry for \"%d, %d, %d\".", tileset, key, letter);
	}

	return *letterIter;
}

bool ArenaTemplateDat::init()
{
	const char *filename = "TEMPLATE.DAT";
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	if (stream == nullptr)
	{
		DebugLogErrorFormat("Could not open \"%s\".", filename);
		return false;
	}

	// Read TEMPLATE.DAT into a string.
	stream->seekg(0, std::ios::end);
	std::string srcText(stream->tellg(), '\0');
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(&srcText.front()), srcText.size());

	// Step line by line through the text, inserting keys and values into the proper lists.
	std::istringstream iss(srcText);
	std::string line, value;
	int key = ArenaTemplateDatEntry::NO_KEY;
	char letter = ArenaTemplateDatEntry::NO_LETTER;

	enum class Mode { None, Key, Section };
	Mode mode = Mode::None;

	auto parseKeyLine = [&key, &letter](const std::string &line)
	{
		// All keys are 4 digits, padded with zeroes. A letter at the end is optional.
		// See if the line has a letter at the end.
		const bool hasLetter = [&line]()
		{
			// Reverse iterate until a non-whitespace character is hit.
			for (auto it = line.rbegin(); it != line.rend(); ++it)
			{
				const char c = *it;

				// If it's a number, we've gone too far and there is no letter.
				if (std::isdigit(c))
				{
					return false;
				}
				// If it's a letter, success.
				else if (std::isalpha(c))
				{
					return true;
				}
			}

			// No letter found.
			return false;
		}();

		// Write out the key string as an integer.
		key = [&line]()
		{
			const int keyOffset = 1;
			const std::string keyStr = line.substr(keyOffset, 4);
			return std::stoi(keyStr);
		}();

		// If there's a letter at the end, write that out too.
		if (hasLetter)
		{
			const int letterIndex = 5;
			DebugAssertIndex(line, letterIndex);
			letter = line[letterIndex];
		}
	};

	auto flushState = [this, &value, &key, &letter]()
	{
		// If no entries yet, create a new vector.
		if (this->entryLists.size() == 0)
		{
			this->entryLists.emplace_back(std::vector<ArenaTemplateDatEntry>());
		}

		// While the current vector contains the given key and optional letter pair, add
		// a new vector to keep tileset-specific strings separate.
		auto containsEntry = [this, key, letter](int i)
		{
			DebugAssertIndex(this->entryLists, i);
			const BufferView<const ArenaTemplateDatEntry> entryList = this->entryLists[i];

			// The entry list might be big (>500 entries) but a linear search shouldn't be
			// very slow when comparing integers. Keeping it sorted during initialization
			// would be too expensive for a std::vector.
			const auto iter = std::find_if(entryList.begin(), entryList.end(),
				[key, letter](const ArenaTemplateDatEntry &entry)
			{
				return (entry.key == key) && ((letter == ArenaTemplateDatEntry::NO_LETTER) || entry.letter == letter);
			});

			return iter != entryList.end();
		};

		int index = 0;
		while (containsEntry(index))
		{
			index++;

			// Create a new vector if necessary.
			if (this->entryLists.size() == index)
			{
				this->entryLists.emplace_back(std::vector<ArenaTemplateDatEntry>());
			}
		}

		// Replace all line breaks with spaces and compress spaces into one.
		std::string trimmedValue = [&value]()
		{
			std::string str;
			char prev = -1;
			for (char c : value)
			{
				if (c == '\r')
				{
					c = ' ';
				}

				if (prev != ' ' || c != ' ')
				{
					str += c;
				}

				prev = c;
			}

			return str;
		}();

		// Trim front and back.
		String::trimFrontInPlace(trimmedValue);
		String::trimBackInPlace(trimmedValue);

		ArenaTemplateDatEntry entry;
		entry.key = key;
		entry.letter = letter;

		Buffer<std::string> trimmedValueTokens = String::split(trimmedValue, '&');
		entry.values = std::vector<std::string>(trimmedValueTokens.getCount());
		std::move(trimmedValueTokens.begin(), trimmedValueTokens.end(), entry.values.begin());

		// Remove unused text after the last ampersand.
		entry.values.pop_back();

		// Add entry to the entry list.
		DebugAssertIndex(this->entryLists, index);
		std::vector<ArenaTemplateDatEntry> outputEntryList = this->entryLists[index];
		outputEntryList.emplace_back(std::move(entry));

		// Reset key, letter, and value string.
		key = ArenaTemplateDatEntry::NO_KEY;
		letter = ArenaTemplateDatEntry::NO_LETTER;
		value.clear();
	};

	while (std::getline(iss, line))
	{
		// Skip empty lines (only for cases where TEMPLATE.DAT is modified to not have '\r'
		// characters, like on Unix, perhaps?).
		if (line.empty())
		{
			continue;
		}

		// See if the line is a key for a section, or if it's a comment.
		const char firstChar = line[0];
		const bool isKeyLine = firstChar == '#';
		const bool isComment = firstChar == ';';

		if (isKeyLine)
		{
			if (mode != Mode::None)
			{
				// The previous line was either a key line or part of a section, so flush it.
				flushState();
			}

			// Read the new key line into the key and optional letter variables.
			parseKeyLine(line);
			mode = Mode::Key;
		}
		else if (isComment)
		{
			// A comment line indicates that the line is skipped and the previous section should
			// be flushed. There's only one comment line in TEMPLATE.DAT at the very end.
			flushState();
			mode = Mode::None;
			continue;
		}
		else if ((mode == Mode::Key) || (mode == Mode::Section))
		{
			// Append the current line onto the value string.
			value.append(line);

			if (mode != Mode::Section)
			{
				mode = Mode::Section;
			}
		}
	}

	// Now that all entry lists have been constructed, sort each one by key, then sort each
	// equal-key sub-group by letter.
	for (BufferView<ArenaTemplateDatEntry> entryList : this->entryLists)
	{
		std::sort(entryList.begin(), entryList.end(),
			[](const ArenaTemplateDatEntry &a, const ArenaTemplateDatEntry &b)
		{
			return a.key < b.key;
		});

		// Find where each equal-key sub-group begins and ends and sort them by letter. In the
		// worst case, the sub-group size will be ~14 entries, so linear search is fine.
		auto beginIter = entryList.begin();
		while (beginIter != entryList.end())
		{
			const auto endIter = std::find_if_not(beginIter, entryList.end(),
				[beginIter](const ArenaTemplateDatEntry &entry)
			{
				return entry.key == beginIter->key;
			});

			std::sort(beginIter, endIter,
				[](const ArenaTemplateDatEntry &a, const ArenaTemplateDatEntry &b)
			{
				return a.letter < b.letter;
			});

			beginIter = endIter;
		}
	}

	return true;
}

bool TextAssetLibrary::initArtifactText()
{
	auto loadArtifactText = [](const char *filename, ArenaArtifactTavernTextArray &artifactTavernTextArray)
	{
		Buffer<std::byte> src;
		if (!VFS::Manager::get().read(filename, &src))
		{
			DebugLogErrorFormat("Could not read \"%s\".", filename);
			return false;
		}

		const char *stringPtr = reinterpret_cast<const char*>(src.begin());
		auto writeNextStrings = [&stringPtr](BufferView<std::string> outStrings)
		{
			for (std::string &str : outStrings)
			{
				str = std::string(stringPtr);
				stringPtr += str.size() + 1;
			}
		};

		for (ArenaArtifactTavernText &textBlock : artifactTavernTextArray)
		{
			writeNextStrings(textBlock.greetingStrs);
			writeNextStrings(textBlock.barterSuccessStrs);
			writeNextStrings(textBlock.offerRefusedStrs);
			writeNextStrings(textBlock.barterFailureStrs);
			writeNextStrings(textBlock.counterOfferStrs);
		}

		return true;
	};

	bool success = loadArtifactText("ARTFACT1.DAT", this->artifactTavernText1);
	success &= loadArtifactText("ARTFACT2.DAT", this->artifactTavernText2);
	return success;
}

bool TextAssetLibrary::initDungeonTxt()
{
	const char *filename = "DUNGEON.TXT";
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogErrorFormat("Could not read \"%s\".", filename);
		return false;
	}

	const std::string text(reinterpret_cast<const char*>(src.begin()), src.getCount());

	// Step line by line through the text, inserting data into the dungeon list.
	std::istringstream iss(text);
	std::string line, title, description;

	while (std::getline(iss, line))
	{
		const char poundSign = '#';

		DebugAssert(!line.empty());
		const char firstChar = line[0];
		if (firstChar == poundSign)
		{
			// Remove the newline from the end of the description.
			if (description.back() == '\n')
			{
				description.pop_back();
			}

			// Put the collected data into the list and restart the title and description.
			this->dungeonTxt.emplace_back(std::make_pair(title, description));
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
				title.replace(titleCarriageReturn, 1, "");
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
				description.replace(descriptionCarriageReturn, 1, "\n");
			}
		}
	}

	return true;
}

bool TextAssetLibrary::initNameChunks()
{
	const char *filename = "NAMECHNK.DAT";
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogErrorFormat("Could not read \"%s\".", filename);
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());

	size_t offset = 0;
	while (offset < src.getCount())
	{
		const uint8_t *chunkPtr = srcPtr + offset;
		const uint16_t chunkLength = Bytes::getLE16(chunkPtr);
		const uint8_t stringCount = *(chunkPtr + 2);

		// Read "stringCount" null-terminated strings.
		size_t stringOffset = 3;
		std::vector<std::string> strings;
		for (int i = 0; i < stringCount; i++)
		{
			const char *stringPtr = reinterpret_cast<const char*>(chunkPtr) + stringOffset;
			strings.emplace_back(std::string(stringPtr));
			stringOffset += strings.back().size() + 1;
		}

		this->nameChunks.emplace_back(std::move(strings));
		offset += chunkLength;
	}

	return true;
}

bool TextAssetLibrary::initQuestionTxt()
{
	const char *filename = "QUESTION.TXT";
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogErrorFormat("Could not read \"%s\".", filename);
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());
	const std::string text(reinterpret_cast<const char*>(srcPtr), src.getCount());

	// Lambda for adding a new question to the questions list.
	auto addQuestion = [this](const std::string &description, const std::string &a, const std::string &b, const std::string &c)
	{
		// Lambda for determining which choices point to which class categories.
		auto getCategory = [](const std::string &choice) -> CharacterClassCategoryID
		{
			const size_t categoryCharBeginIndex = choice.find("(5");
			if (categoryCharBeginIndex == std::string::npos)
			{
				DebugLogErrorFormat("Couldn't find category char begin index in \"%s\".", choice.c_str());
				return -1;
			}

			// l: Logical (mage)
			// c: Clever (thief)
			// v: Violent (warrior)
			constexpr char mageLetter = 'l';
			constexpr char thiefLetter = 'c';
			constexpr char warriorLetter = 'v';
			constexpr char categoryChars[3] = { mageLetter, thiefLetter, warriorLetter };

			const size_t categoryCharIndex = choice.find_first_of(categoryChars, categoryCharBeginIndex + 2);
			if (categoryCharIndex == std::string::npos)
			{
				DebugLogErrorFormat("Couldn't find category char index in \"%s\".", choice.c_str());
				return -1;
			}

			const char categoryChar = choice[categoryCharIndex];
			for (int i = 0; i < static_cast<int>(std::size(categoryChars)); i++)
			{
				if (categoryChar == categoryChars[i])
				{
					return i;
				}
			}

			DebugLogErrorFormat("Couldn't find matching category ID for char \"%c\".", categoryChar);
			return -1;
		};

		CharacterQuestionChoice questionChoiceA;
		CharacterQuestionChoice questionChoiceB;
		CharacterQuestionChoice questionChoiceC;
		questionChoiceA.init(a.c_str(), getCategory(a));
		questionChoiceB.init(b.c_str(), getCategory(b));
		questionChoiceC.init(c.c_str(), getCategory(c));

		CharacterQuestion question;
		question.init(description.c_str(), questionChoiceA, questionChoiceB, questionChoiceC);
		this->questionTxt.emplace_back(std::move(question));
	};

	// Step line by line through the text, creating question objects.
	std::istringstream iss(text);
	std::string line, description, a, b, c;

	enum class Mode { Description, A, B, C };
	Mode mode = Mode::Description;

	while (std::getline(iss, line))
	{
		DebugAssert(!line.empty());
		const unsigned char firstChar = static_cast<unsigned char>(line[0]);

		if (std::isalpha(firstChar))
		{
			// See if it's 'a', 'b', or 'c', and switch to that mode.
			if (firstChar == 'a')
			{
				mode = Mode::A;
			}
			else if (firstChar == 'b')
			{
				mode = Mode::B;
			}
			else if (firstChar == 'c')
			{
				mode = Mode::C;
			}
		}
		else if (std::isdigit(firstChar))
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
	return true;
}

bool TextAssetLibrary::initSpellMakerDescriptions()
{
	const char *filename = "SPELLMKR.TXT";
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogErrorFormat("Could not read \"%s\".", filename);
		return false;
	}

	const std::string text(reinterpret_cast<const char*>(src.begin()), src.getCount());

	struct State
	{
		int index;
		std::string str;

		State(int index)
		{
			this->index = index;
		}
	};

	std::unique_ptr<State> state;
	std::stringstream ss(text);
	std::string line;

	while (std::getline(ss, line))
	{
		if (line.size() > 0)
		{
			const char firstChar = line.front();
			const char INDEX_CHAR = '#';

			if (firstChar == INDEX_CHAR)
			{
				// Flush any existing state.
				if (state.get() != nullptr)
				{
					DebugAssertIndex(this->spellMakerDescriptions, state->index);
					this->spellMakerDescriptions[state->index] = std::move(state->str);
					state = nullptr;
				}

				// If there's an index in the line, it's valid. Otherwise, break.
				const bool containsIndex = line.size() >= 3;
				if (containsIndex)
				{
					const int index = std::stoi(line.substr(1, 2));
					state = std::make_unique<State>(index);
				}
				else
				{
					break;
				}
			}
			else
			{
				// Read text into the existing state.
				state->str += line;
			}
		}
	}

	return true;
}

bool TextAssetLibrary::initTemplateDat()
{
	return this->templateDat.init();
}

bool TextAssetLibrary::initTradeText()
{
	auto loadTradeText = [](const char *filename, ArenaTradeText::FunctionArray &functionArr)
	{
		Buffer<std::byte> src;
		if (!VFS::Manager::get().read(filename, &src))
		{
			DebugLogErrorFormat("Could not read \"%s\".", filename);
			return false;
		}

		const char *stringPtr = reinterpret_cast<const char*>(src.begin());

		// Write the null-terminated strings to the output array.
		for (ArenaTradeText::PersonalityArray &personalityArr : functionArr)
		{
			for (ArenaTradeText::RandomArray &randomArr : personalityArr)
			{
				for (std::string &str : randomArr)
				{
					str = std::string(stringPtr);
					stringPtr += str.size() + 1;
				}
			}
		}

		return true;
	};

	bool success = loadTradeText("EQUIP.DAT", this->tradeText.equipment);
	success &= loadTradeText("MUGUILD.DAT", this->tradeText.magesGuild);
	success &= loadTradeText("SELLING.DAT", this->tradeText.selling);
	success &= loadTradeText("TAVERN.DAT", this->tradeText.tavern);
	return success;
}

bool TextAssetLibrary::init()
{
	DebugLog("Initializing text assets.");
	bool success = this->initArtifactText();
	success &= this->initDungeonTxt();
	success &= this->initNameChunks();
	success &= this->initQuestionTxt();
	success &= this->initSpellMakerDescriptions();
	success &= this->initTemplateDat();
	success &= this->initTradeText();
	return success;
}

std::string TextAssetLibrary::generateNpcName(int raceID, bool isMale, ArenaRandom &random) const
{
	const int nameRuleIndex = (raceID * 2) + (isMale ? 0 : 1);
	DebugAssertIndex(NameRules, nameRuleIndex);
	const BufferView<const NameRule> &chunkRules = NameRules[nameRuleIndex];

	// Construct the name from each part of the rule.
	std::string name;
	for (const NameRule &rule : chunkRules)
	{
		if (rule.type == NameRuleType::Index)
		{
			DebugAssertIndex(this->nameChunks, rule.index);
			const ArenaNameChunkEntry &chunkList = this->nameChunks[rule.index];
			const int chunkListIndex = DebugMakeIndex(chunkList, random.next() % static_cast<int>(chunkList.size()));
			name += chunkList[chunkListIndex];
		}
		else if (rule.type == NameRuleType::String)
		{
			name += std::string(rule.str.data());
		}
		else if (rule.type == NameRuleType::IndexChance)
		{
			DebugAssertIndex(this->nameChunks, rule.indexChance.index);
			const ArenaNameChunkEntry &chunkList = this->nameChunks[rule.indexChance.index];
			if ((random.next() % 100) <= rule.indexChance.chance)
			{
				const int chunkListIndex = DebugMakeIndex(chunkList, random.next() % static_cast<int>(chunkList.size()));
				name += chunkList[chunkListIndex];
			}
		}
		else if (rule.type == NameRuleType::IndexStringChance)
		{
			DebugAssertIndex(this->nameChunks, rule.indexStringChance.index);
			const ArenaNameChunkEntry &chunkList = this->nameChunks[rule.indexStringChance.index];
			if ((random.next() % 100) <= rule.indexStringChance.chance)
			{
				const int chunkListIndex = DebugMakeIndex(chunkList, random.next() % static_cast<int>(chunkList.size()));
				name += chunkList[chunkListIndex] + std::string(rule.indexStringChance.str.data());
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(rule.type)));
		}
	}

	return name;
}
