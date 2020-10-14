#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>

#include "TextAssetLibrary.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/Buffer.h"
#include "components/utilities/String.h"
#include "components/vfs/manager.hpp"

namespace
{
	// Discriminated union for name composition rules used with NAMECHNK.DAT.
	// Each rule is either:
	// - Index
	// - Pre-defined string
	// - Index with chance
	// - Index and string with chance
	struct NameRule
	{
		enum class Type
		{
			Index, // Points into chunk lists.
			String, // Pre-defined string.
			IndexChance, // Points into chunk lists, with a chance to not be used.
			IndexStringChance, // Points into chunk lists, w/ string and chance.
		};

		struct IndexChance
		{
			int index, chance;
		};

		struct IndexStringChance
		{
			int index;
			std::array<char, 4> str;
			int chance;
		};

		NameRule::Type type;

		union
		{
			int index;
			std::array<char, 4> str;
			IndexChance indexChance;
			IndexStringChance indexStringChance;
		};

		NameRule(int index)
		{
			this->type = Type::Index;
			this->index = index;
		}

		NameRule(const std::string &str)
		{
			this->type = Type::String;
			this->str.fill('\0');
			const size_t charCount = std::min(str.size(), this->str.size());
			std::copy(str.begin(), str.begin() + charCount, this->str.begin());
		}

		NameRule(int index, int chance)
		{
			this->type = Type::IndexChance;
			this->indexChance.index = index;
			this->indexChance.chance = chance;
		}

		NameRule(int index, const std::string &str, int chance)
		{
			this->type = Type::IndexStringChance;
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

const TextAssetLibrary::TemplateDat::Entry &TextAssetLibrary::TemplateDat::getEntry(int key) const
{
	// Use first vector for non-tileset entry requests.
	const auto &entryList = this->entryLists.at(0);

	const auto iter = std::lower_bound(entryList.begin(), entryList.end(), key,
		[](const Entry &a, int key)
	{
		return a.key < key;
	});

	if (iter == entryList.end())
	{
		DebugCrash("No TEMPLATE.DAT entry for \"" + std::to_string(key) + "\".");
	}

	return *iter;
}

const TextAssetLibrary::TemplateDat::Entry &TextAssetLibrary::TemplateDat::getEntry(
	int key, char letter) const
{
	// Use first vector for non-tileset entry requests.
	const auto &entryList = this->entryLists.at(0);

	// The requested entry has a letter in its key, so need to find the range of
	// equal values for 'key' via binary search.
	const auto lowerIter = std::lower_bound(entryList.begin(), entryList.end(), key,
		[](const Entry &a, int key)
	{
		return a.key < key;
	});

	const auto upperIter = std::upper_bound(lowerIter, entryList.end(), key,
		[](int key, const Entry &b)
	{
		return key < b.key;
	});

	// Find 'letter' in the range of equal key values.
	const auto iter = std::lower_bound(lowerIter, upperIter, letter,
		[](const Entry &a, char letter)
	{
		return a.letter < letter;
	});

	if (iter == upperIter)
	{
		DebugCrash("No TEMPLATE.DAT entry for \"" + std::to_string(key) + ", " +
			std::to_string(static_cast<int>(letter)) + "\".");
	}

	return *iter;
}

const TextAssetLibrary::TemplateDat::Entry &TextAssetLibrary::TemplateDat::getTilesetEntry(
	int tileset, int key, char letter) const
{
	const auto &entryList = this->entryLists.at(tileset);

	// Do binary search in the tileset vector to find the equal range for 'key'.
	const auto lowerIter = std::lower_bound(entryList.begin(), entryList.end(), key,
		[](const Entry &a, int key)
	{
		return a.key < key;
	});

	const auto upperIter = std::upper_bound(lowerIter, entryList.end(), key,
		[](int key, const Entry &b)
	{
		return key < b.key;
	});

	// Find 'letter' in the range of equal key values.
	const auto iter = std::lower_bound(lowerIter, upperIter, letter,
		[](const Entry &a, char letter)
	{
		return a.letter < letter;
	});

	if (iter == upperIter)
	{
		DebugCrash("No TEMPLATE.DAT entry for \"" + std::to_string(tileset) + ", " +
			std::to_string(key) + ", " + std::to_string(static_cast<int>(letter)) + "\".");
	}

	return *iter;
}

bool TextAssetLibrary::TemplateDat::init()
{
	const char *filename = "TEMPLATE.DAT";
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	if (stream == nullptr)
	{
		DebugLogError("Could not open \"" + std::string(filename) + "\".");
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
	int key = Entry::NO_KEY;
	char letter = Entry::NO_LETTER;

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
			letter = line.at(letterIndex);
		}
	};

	auto flushState = [this, &value, &key, &letter]()
	{
		// If no entries yet, create a new vector.
		if (this->entryLists.size() == 0)
		{
			this->entryLists.push_back(std::vector<Entry>());
		}

		// While the current vector contains the given key and optional letter pair, add
		// a new vector to keep tileset-specific strings separate.
		auto containsEntry = [this, key, letter](int i)
		{
			const auto &entryList = this->entryLists.at(i);

			// The entry list might be big (>500 entries) but a linear search shouldn't be
			// very slow when comparing integers. Keeping it sorted during initialization
			// would be too expensive for a std::vector.
			const auto iter = std::find_if(entryList.begin(), entryList.end(),
				[key, letter](const Entry &entry)
			{
				return (entry.key == key) &&
					((letter == Entry::NO_LETTER) || entry.letter == letter);
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
				this->entryLists.push_back(std::vector<Entry>());
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

		Entry entry;
		entry.key = key;
		entry.letter = letter;
		entry.values = String::split(trimmedValue, '&');

		// Remove unused text after the last ampersand.
		entry.values.pop_back();

		// Add entry to the entry list.
		this->entryLists.at(index).push_back(std::move(entry));

		// Reset key, letter, and value string.
		key = Entry::NO_KEY;
		letter = Entry::NO_LETTER;
		value.clear();
	};

	while (std::getline(iss, line))
	{
		// Skip empty lines (only for cases where TEMPLATE.DAT is modified to not have '\r'
		// characters, like on Unix, perhaps?).
		if (line.size() == 0)
		{
			continue;
		}

		// See if the line is a key for a section, or if it's a comment.
		const bool isKeyLine = line.at(0) == '#';
		const bool isComment = line.at(0) == ';';

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
	for (auto &entryList : this->entryLists)
	{
		std::sort(entryList.begin(), entryList.end(),
			[](const Entry &a, const Entry &b)
		{
			return a.key < b.key;
		});

		// Find where each equal-key sub-group begins and ends and sort them by letter. In the
		// worst case, the sub-group size will be ~14 entries, so linear search is fine.
		auto beginIter = entryList.begin();
		while (beginIter != entryList.end())
		{
			const auto endIter = std::find_if_not(beginIter, entryList.end(),
				[beginIter](const Entry &entry)
			{
				return entry.key == beginIter->key;
			});

			std::sort(beginIter, endIter,
				[](const Entry &a, const Entry &b)
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
	auto loadArtifactText = [](const char *filename,
		TextAssetLibrary::ArtifactTavernTextArray &artifactTavernTextArray)
	{
		Buffer<std::byte> src;
		if (!VFS::Manager::get().read(filename, &src))
		{
			DebugLogError("Could not read \"" + std::string(filename) + "\".");
			return false;
		}

		// Write the null-terminated strings to the output array.
		const char *stringPtr = reinterpret_cast<const char*>(src.get());
		for (auto &block : artifactTavernTextArray)
		{
			auto initStringArray = [&stringPtr](std::array<std::string, 3> &arr)
			{
				for (std::string &str : arr)
				{
					str = std::string(stringPtr);
					stringPtr += str.size() + 1;
				}
			};

			initStringArray(block.greetingStrs);
			initStringArray(block.barterSuccessStrs);
			initStringArray(block.offerRefusedStrs);
			initStringArray(block.barterFailureStrs);
			initStringArray(block.counterOfferStrs);
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
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const std::string text(reinterpret_cast<const char*>(src.get()), src.getCount());

	// Step line by line through the text, inserting data into the dungeon list.
	std::istringstream iss(text);
	std::string line, title, description;

	while (std::getline(iss, line))
	{
		const char poundSign = '#';
		if (line.at(0) == poundSign)
		{
			// Remove the newline from the end of the description.
			if (description.back() == '\n')
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
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.get());

	size_t offset = 0;
	while (offset < src.getCount())
	{
		// Get information for the current chunk.
		const uint8_t *chunkPtr = srcPtr + offset;
		const uint16_t chunkLength = Bytes::getLE16(chunkPtr);
		const uint8_t stringCount = *(chunkPtr + 2);

		// Read "stringCount" null-terminated strings.
		size_t stringOffset = 3;
		std::vector<std::string> strings;
		for (int i = 0; i < stringCount; i++)
		{
			const char *stringPtr = reinterpret_cast<const char*>(chunkPtr) + stringOffset;
			strings.push_back(std::string(stringPtr));
			stringOffset += strings.back().size() + 1;
		}

		this->nameChunks.push_back(std::move(strings));
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
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.get());

	// Read QUESTION.TXT into a string.
	const std::string text(reinterpret_cast<const char*>(srcPtr), src.getCount());

	// Lambda for adding a new question to the questions list.
	auto addQuestion = [this](const std::string &description,
		const std::string &a, const std::string &b, const std::string &c)
	{
		// Lambda for determining which choices point to which class categories.
		auto getCategory = [](const std::string &choice) -> CharacterClassDefinition::CategoryID
		{
			const char mageChar = 'l'; // Logical?
			const char thiefChar = 'c'; // Clever?
			const char warriorChar = 'v'; // Violent?
			const char categoryChar = choice.at(choice.find("(5") + 2);

			if (categoryChar == mageChar)
			{
				return 0;
			}
			else if (categoryChar == thiefChar)
			{
				return 1;
			}
			else if (categoryChar == warriorChar)
			{
				return 2;
			}
			else
			{
				// @todo: redesign error-handling via bools so we don't need exceptions
				// for file correctness.
				throw DebugException("Bad QUESTION.TXT class category.");
			}
		};

		this->questionTxt.push_back(CharacterQuestion(std::string(description),
			std::make_pair(a, getCategory(a)), std::make_pair(b, getCategory(b)),
			std::make_pair(c, getCategory(c))));
	};

	// Step line by line through the text, creating question objects.
	std::istringstream iss(text);
	std::string line, description, a, b, c;

	enum class Mode { Description, A, B, C };
	Mode mode = Mode::Description;

	while (std::getline(iss, line))
	{
		const unsigned char ch = static_cast<unsigned char>(line.at(0));

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
	return true;
}

bool TextAssetLibrary::initSpellMakerDescriptions()
{
	const char *filename = "SPELLMKR.TXT";
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const std::string text(reinterpret_cast<const char*>(src.get()), src.getCount());

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
					this->spellMakerDescriptions.at(state->index) = std::move(state->str);
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
	auto loadTradeText = [](const char *filename,
		TextAssetLibrary::TradeText::FunctionArray &functionArr)
	{
		Buffer<std::byte> src;
		if (!VFS::Manager::get().read(filename, &src))
		{
			DebugLogError("Could not read \"" + std::string(filename) + "\".");
			return false;
		}

		// Write the null-terminated strings to the output array.
		const char *stringPtr = reinterpret_cast<const char*>(src.get());
		for (TextAssetLibrary::TradeText::PersonalityArray &personalityArr : functionArr)
		{
			for (TextAssetLibrary::TradeText::RandomArray &randomArr : personalityArr)
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

const TextAssetLibrary::ArtifactTavernTextArray &TextAssetLibrary::getArtifactTavernText1() const
{
	return this->artifactTavernText1;
}

const TextAssetLibrary::ArtifactTavernTextArray &TextAssetLibrary::getArtifactTavernText2() const
{
	return this->artifactTavernText2;
}

const std::vector<TextAssetLibrary::DungeonTxtEntry> &TextAssetLibrary::getDungeonTxtDungeons() const
{
	return this->dungeonTxt;
}

const std::vector<CharacterQuestion> &TextAssetLibrary::getQuestionTxtQuestions() const
{
	return this->questionTxt;
}

const TextAssetLibrary::SpellMakerDescriptionArray &TextAssetLibrary::getSpellMakerDescriptions() const
{
	return this->spellMakerDescriptions;
}

const TextAssetLibrary::TemplateDat &TextAssetLibrary::getTemplateDat() const
{
	return this->templateDat;
}

const TextAssetLibrary::TradeText &TextAssetLibrary::getTradeText() const
{
	return this->tradeText;
}

std::string TextAssetLibrary::generateNpcName(int raceID, bool isMale, ArenaRandom &random) const
{
	// Get the rules associated with the race and gender.
	const int nameRuleIndex = DebugMakeIndex(NameRules, (raceID * 2) + (isMale ? 0 : 1));
	const std::vector<NameRule> &chunkRules = NameRules[nameRuleIndex];

	// Construct the name from each part of the rule.
	std::string name;
	for (const NameRule &rule : chunkRules)
	{
		if (rule.type == NameRule::Type::Index)
		{
			DebugAssertIndex(this->nameChunks, rule.index);
			const NameChunkEntry &chunkList = this->nameChunks[rule.index];
			const int chunkListIndex = DebugMakeIndex(
				chunkList, random.next() % static_cast<int>(chunkList.size()));
			name += chunkList[chunkListIndex];
		}
		else if (rule.type == NameRule::Type::String)
		{
			name += std::string(rule.str.data());
		}
		else if (rule.type == NameRule::Type::IndexChance)
		{
			DebugAssertIndex(this->nameChunks, rule.indexChance.index);
			const NameChunkEntry &chunkList = this->nameChunks[rule.indexChance.index];
			if ((random.next() % 100) <= rule.indexChance.chance)
			{
				const int chunkListIndex = DebugMakeIndex(
					chunkList, random.next() % static_cast<int>(chunkList.size()));
				name += chunkList[chunkListIndex];
			}
		}
		else if (rule.type == NameRule::Type::IndexStringChance)
		{
			DebugAssertIndex(this->nameChunks, rule.indexStringChance.index);
			const NameChunkEntry &chunkList = this->nameChunks[rule.indexStringChance.index];
			if ((random.next() % 100) <= rule.indexStringChance.chance)
			{
				const int chunkListIndex = DebugMakeIndex(
					chunkList, random.next() % static_cast<int>(chunkList.size()));
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
