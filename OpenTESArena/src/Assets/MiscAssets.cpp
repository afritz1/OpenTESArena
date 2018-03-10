#include <algorithm>
#include <cassert>
#include <cctype>
#include <numeric>
#include <sstream>

#include "MiscAssets.h"
#include "../Entities/CharacterClassCategoryName.h"
#include "../Items/ArmorMaterialType.h"
#include "../Items/ShieldType.h"
#include "../Math/Random.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/Platform.h"
#include "../Utilities/String.h"

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
			this->index = index;
			this->type = Type::Index;
		}

		NameRule(const std::string &str)
		{
			this->str.fill('\0');
			const size_t charCount = std::min(str.size(), this->str.size());
			std::copy(str.begin(), str.begin() + charCount, this->str.begin());

			this->type = Type::String;
		}

		NameRule(int index, int chance)
		{
			this->indexChance.index = index;
			this->indexChance.chance = chance;
			this->type = Type::IndexChance;
		}

		NameRule(int index, const std::string &str, int chance)
		{
			this->indexStringChance.index = index;

			this->indexStringChance.str.fill('\0');
			const size_t charCount = std::min(str.size(), this->indexStringChance.str.size());
			std::copy(str.begin(), str.begin() + charCount, this->indexStringChance.str.begin());

			this->indexStringChance.chance = chance;

			this->type = Type::IndexStringChance;
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

MiscAssets::MiscAssets()
{
	// Initialized by init().
}

MiscAssets::~MiscAssets()
{

}

void MiscAssets::init()
{
	// Load the executable data.
	this->parseExecutableData();

	// Read in TEMPLATE.DAT, using "#..." as keys and the text as values.
	this->parseTemplateDat();

	// Read in QUESTION.TXT and create character question objects.
	this->parseQuestionTxt();

	// Read in CLASSES.DAT.
	this->parseClasses(this->getExeData());

	// Read in DUNGEON.TXT and pair each dungeon name with its description.
	this->parseDungeonTxt();

	// Read in ARTFACT1.DAT and ARTFACT2.DAT.
	this->parseArtifactText();

	// Read in EQUIP.DAT, MUGUILD.DAT, SELLING.DAT, and TAVERN.DAT.
	this->parseTradeText();

	// Read in NAMECHNK.DAT.
	this->parseNameChunks();

	// Read city data file.
	this->cityDataFile.init("CITYDATA.00");

	// Read in the world map mask data from TAMRIEL.MNU.
	this->parseWorldMapMasks();
}

void MiscAssets::parseExecutableData()
{
	// For now, just read the floppy disk executable.
	const bool floppyVersion = true;
	this->exeData.init(floppyVersion);
}

void MiscAssets::parseTemplateDat()
{
	const std::string filename = "TEMPLATE.DAT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Read TEMPLATE.DAT into a string.
	const std::string text(reinterpret_cast<const char*>(srcData.data()), srcData.size());

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

				while ((value.size() > 0) && (value.back() == '\n'))
				{
					value.pop_back();
				}

				// Remove the annoying ampersand at the end of most texts.
				if ((value.size() > 0) && (value.back() == '&'))
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

void MiscAssets::parseQuestionTxt()
{
	const std::string filename = "QUESTION.TXT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Read QUESTION.TXT into a string.
	const std::string text(reinterpret_cast<const char*>(srcData.data()), srcData.size());

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

void MiscAssets::parseClasses(const ExeData &exeData)
{
	const std::string filename = "CLASSES.DAT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Character class generation members (to be set).
	auto &classes = this->classesDat.classes;
	auto &choices = this->classesDat.choices;

	// The class IDs take up the first 18 bytes.
	for (size_t i = 0; i < classes.size(); i++)
	{
		const uint8_t *srcPtr = srcData.data() + i;
		const uint8_t value = *srcPtr;

		CharacterClassGeneration::ClassData &classData = classes.at(i);
		classData.id = value & CharacterClassGeneration::ID_MASK;
		classData.isSpellcaster = (value & CharacterClassGeneration::SPELLCASTER_MASK) != 0;
		classData.hasCriticalHit = (value & CharacterClassGeneration::CRITICAL_HIT_MASK) != 0;
		classData.isThief = (value & CharacterClassGeneration::THIEF_MASK) != 0;
	}

	// After the class IDs are 66 groups of "A, B, C" choices. They account for all 
	// the combinations of answers to character questions. When the user is done
	// answering questions, their A/B/C counts map to some index in the Choices array.
	for (size_t i = 0; i < choices.size(); i++)
	{
		const int choiceSize = 3;
		const uint8_t *srcPtr = srcData.data() + classes.size() + (choiceSize * i);

		CharacterClassGeneration::ChoiceData &choice = choices.at(i);
		choice.a = *srcPtr;
		choice.b = *(srcPtr + 1);
		choice.c = *(srcPtr + 2);
	}

	// Now read in the character class data from A.EXE. Some of it also depends on
	// data from CLASSES.DAT.
	const auto &classNameStrs = exeData.charClasses.classNames;
	const auto &allowedArmorsValues = exeData.charClasses.allowedArmors;
	const auto &allowedShieldsLists = exeData.charClasses.allowedShieldsLists;
	const auto &allowedShieldsIndices = exeData.charClasses.allowedShieldsIndices;
	const auto &allowedWeaponsLists = exeData.charClasses.allowedWeaponsLists;
	const auto &allowedWeaponsIndices = exeData.charClasses.allowedWeaponsIndices;
	const auto &preferredAttributesStrs = exeData.charClasses.preferredAttributes;
	const auto &classNumbersToIDsValues = exeData.charClasses.classNumbersToIDs;
	const auto &initialExpCapValues = exeData.charClasses.initialExperienceCaps;
	const auto &healthDiceValues = exeData.charClasses.healthDice;
	const auto &lockpickingDivisorValues = exeData.charClasses.lockpickingDivisors;

	const int classCount = 18;
	for (int i = 0; i < classCount; i++)
	{
		const std::string &name = classNameStrs.at(i);
		const std::string &preferredAttributes = preferredAttributesStrs.at(i);

		const std::vector<ArmorMaterialType> allowedArmors = [&allowedArmorsValues, i]()
		{
			// Determine which armors are allowed based on a one-digit value.
			const uint8_t value = allowedArmorsValues.at(i);

			if (value == 0)
			{
				return std::vector<ArmorMaterialType>
				{
					ArmorMaterialType::Leather, ArmorMaterialType::Chain, ArmorMaterialType::Plate
				};
			}
			else if (value == 1)
			{
				return std::vector<ArmorMaterialType>
				{
					ArmorMaterialType::Leather, ArmorMaterialType::Chain
				};
			}
			else if (value == 2)
			{
				return std::vector<ArmorMaterialType>
				{
					ArmorMaterialType::Leather
				};
			}
			else if (value == 3)
			{
				return std::vector<ArmorMaterialType>();
			}
			else
			{
				throw std::runtime_error("Bad allowed armors value \"" +
					std::to_string(value) + "\".");
			}
		}();

		const std::vector<ShieldType> allowedShields = [&allowedShieldsLists,
			&allowedShieldsIndices, i]()
		{
			// Get the pre-calculated shield index.
			const int shieldIndex = allowedShieldsIndices.at(i);
			const int NO_INDEX = -1;

			// If the index is "null" (-1), that means all shields are allowed for this class.
			if (shieldIndex == NO_INDEX)
			{
				return std::vector<ShieldType>
				{
					ShieldType::Buckler, ShieldType::Round, ShieldType::Kite, ShieldType::Tower
				};
			}
			else
			{
				// Mappings of shield IDs to shield types. The index in the array is the ID 
				// minus 7 because shields and armors are treated as the same type in Arena,
				// so they're in the same array, but we separate them here because that seems 
				// more object-oriented.
				const std::array<ShieldType, 4> ShieldIDMappings =
				{
					ShieldType::Buckler,
					ShieldType::Round,
					ShieldType::Kite,
					ShieldType::Tower
				};

				const std::vector<uint8_t> &shieldsList = allowedShieldsLists.at(shieldIndex);
				std::vector<ShieldType> shields;

				for (const uint8_t shield : shieldsList)
				{
					shields.push_back(ShieldIDMappings.at(shield - 7));
				}

				return shields;
			}
		}();

		const std::vector<int> allowedWeapons = [&allowedWeaponsLists,
			&allowedWeaponsIndices, i]()
		{
			// Get the pre-calculated weapon index.
			const int weaponIndex = allowedWeaponsIndices.at(i);
			const int NO_INDEX = -1;

			// Weapon IDs as they are shown in the executable (staff, sword, ..., long bow).
			const std::vector<int> WeaponIDs = []()
			{
				std::vector<int> weapons(18);
				std::iota(weapons.begin(), weapons.end(), 0);
				return weapons;
			}();

			// If the index is "null" (-1), that means all weapons are allowed for this class.
			if (weaponIndex == NO_INDEX)
			{
				return WeaponIDs;
			}
			else
			{
				const std::vector<uint8_t> &weaponsList = allowedWeaponsLists.at(weaponIndex);
				std::vector<int> weapons;

				for (const uint8_t weapon : weaponsList)
				{
					weapons.push_back(WeaponIDs.at(weapon));
				}

				return weapons;
			}
		}();

		const CharacterClassCategoryName categoryName = [i]()
		{
			if (i < 6)
			{
				return CharacterClassCategoryName::Mage;
			}
			else if (i < 12)
			{
				return CharacterClassCategoryName::Thief;
			}
			else
			{
				return CharacterClassCategoryName::Warrior;
			}
		}();

		const double lockpicking = [&lockpickingDivisorValues, i]()
		{
			const uint8_t divisor = lockpickingDivisorValues.at(i);
			return static_cast<double>(200 / divisor) / 100.0;
		}();

		const int healthDie = healthDiceValues.at(i);
		const int initialExperienceCap = initialExpCapValues.at(i);
		const int classNumberToID = classNumbersToIDsValues.at(i);

		const int classIndex = classNumberToID & CharacterClassGeneration::ID_MASK;
		const bool mage = (classNumberToID & CharacterClassGeneration::SPELLCASTER_MASK) != 0;
		const bool thief = (classNumberToID & CharacterClassGeneration::THIEF_MASK) != 0;
		const bool criticalHit = (classNumberToID & CharacterClassGeneration::CRITICAL_HIT_MASK) != 0;

		this->classDefinitions.push_back(CharacterClass(name, preferredAttributes,
			allowedArmors, allowedShields, allowedWeapons, categoryName, lockpicking,
			healthDie, initialExperienceCap, classIndex, mage, thief, criticalHit));
	}
}

void MiscAssets::parseDungeonTxt()
{
	const std::string filename = "DUNGEON.TXT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	const std::string text(reinterpret_cast<const char*>(srcData.data()), srcData.size());

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

void MiscAssets::parseArtifactText()
{
	auto loadArtifactText = [](const std::string &filename, MiscAssets::ArtifactText &artifactText)
	{
		VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
		DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

		stream->seekg(0, std::ios::end);
		std::vector<uint8_t> srcData(stream->tellg());
		stream->seekg(0, std::ios::beg);
		stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

		const std::string text(reinterpret_cast<const char*>(srcData.data()), srcData.size());
		const size_t separatorLength = 4;

		// Have to search for the separator string this way because storing it
		// in a string causes it to only see the first exclamation mark.
		auto atSeparator = [&text](size_t index)
		{
			return (text.at(index) == '!') &&
				(text.at(index + 1) == '\0') &&
				(text.at(index + 2) == '!') &&
				(text.at(index + 3) == '\0');
		};

		// Searches forwards for "!\0!\0" from the offset.
		auto nextSeparator = [&text, &atSeparator](size_t offset)
		{
			size_t index = text.find('!', offset);
			while (!atSeparator(index))
			{
				index = text.find('!', index + 1);
			}

			return index;
		};

		// General case: read 15 artifacts, starting after the first separator.
		const char *stringPtr = text.data() + nextSeparator(0) + separatorLength;

		auto initStringArray = [&stringPtr](std::array<std::string, 3> &arr)
		{
			for (std::string &str : arr)
			{
				str = std::string(stringPtr);
				stringPtr += str.size() + 1;
			}
		};

		for (size_t i = 1; i < artifactText.chunks.size(); i++)
		{
			auto &chunk = artifactText.chunks.at(i);

			initStringArray(chunk.playerTooGreedy);
			initStringArray(chunk.npcQuits);
			initStringArray(chunk.npcCountersOffers);
			initStringArray(chunk.npcGreets);

			chunk.accept = std::string(stringPtr);
			stringPtr += chunk.accept.size() + 1;
			
			// Move pointer to the next chunk.
			stringPtr = text.data() + nextSeparator(stringPtr - text.data()) + separatorLength;
		}

		// Searches backwards for "!\0!\0" from the offset.
		auto prevSeparator = [&text, &atSeparator](size_t offset)
		{
			size_t index = text.rfind('!', offset);
			while (!atSeparator(index))
			{
				index = text.rfind('!', index - 1);
			}

			return index;
		};

		// Special case: read the first artifact, split between the front and back of
		// the file.
		auto &firstChunk = artifactText.chunks.front();

		stringPtr = text.data();
		initStringArray(firstChunk.npcGreets);
		firstChunk.accept = std::string(stringPtr);

		stringPtr = text.data() + prevSeparator(text.size() - 1) + separatorLength;
		initStringArray(firstChunk.playerTooGreedy);
		initStringArray(firstChunk.npcQuits);
		initStringArray(firstChunk.npcCountersOffers);
	};

	loadArtifactText("ARTFACT1.DAT", this->artifactText1);
	loadArtifactText("ARTFACT2.DAT", this->artifactText2);
}

void MiscAssets::parseTradeText()
{
	auto loadTradeText = [](const std::string &filename,
		MiscAssets::TradeText::FunctionArray &functionArr)
	{
		VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
		DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

		stream->seekg(0, std::ios::end);
		std::vector<uint8_t> srcData(stream->tellg());
		stream->seekg(0, std::ios::beg);
		stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

		// Write the null-terminated strings to the output array.
		const char *stringPtr = reinterpret_cast<const char*>(srcData.data());
		for (MiscAssets::TradeText::PersonalityArray &personalityArr : functionArr)
		{
			for (MiscAssets::TradeText::RandomArray &randomArr : personalityArr)
			{
				for (std::string &str : randomArr)
				{
					str = std::string(stringPtr);
					stringPtr += str.size() + 1;
				}
			}
		}
	};

	loadTradeText("EQUIP.DAT", this->tradeText.equipment);
	loadTradeText("MUGUILD.DAT", this->tradeText.magesGuild);
	loadTradeText("SELLING.DAT", this->tradeText.selling);
	loadTradeText("TAVERN.DAT", this->tradeText.tavern);
}

void MiscAssets::parseNameChunks()
{
	const std::string filename("NAMECHNK.DAT");

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	size_t offset = 0;
	while (offset < srcData.size())
	{
		// Get information for the current chunk.
		const uint8_t *chunkPtr = srcData.data() + offset;
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
}

void MiscAssets::parseWorldMapMasks()
{
	const std::string filename = "TAMRIEL.MNU";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Beginning of the mask data.
	const int startOffset = 0x87D5;

	// Each province's mask rectangle is a set of bits packed together with others.
	const std::array<Rect, 10> MaskRects =
	{
		Rect(37, 32, 86, 57),
		Rect(47, 53, 90, 62),
		Rect(113, 29, 88, 53),
		Rect(190, 31, 102, 93),
		Rect(31, 131, 65, 52),
		Rect(100, 118, 61, 55),
		Rect(144, 119, 50, 57),
		Rect(204, 116, 67, 67),
		Rect(103, 72, 131, 84),
		Rect(279, 188, 37, 11) // "Exit" button.
	};

	// Initialize each of the world map masks, moving the offset to the beginning
	// of the next data each loop.
	int offset = 0;
	for (size_t i = 0; i < this->worldMapMasks.size(); i++)
	{
		const Rect &rect = MaskRects.at(i);

		// The number of bytes in the mask rect.
		const int byteCount =
			WorldMapMask::getAdjustedWidth(rect.getWidth()) * rect.getHeight();

		// Copy the segment of mask bytes to a new vector.
		const auto maskStart = srcData.begin() + startOffset + offset;
		const auto maskEnd = maskStart + byteCount;
		std::vector<uint8_t> maskData(maskStart, maskEnd);

		// Assign the map mask onto the map masks list.
		this->worldMapMasks.at(i) = WorldMapMask(std::move(maskData), rect);

		// Move to the next mask.
		offset += byteCount;
	}
}

const ExeData &MiscAssets::getExeData() const
{
	return this->exeData;
}

const std::string &MiscAssets::getTemplateDatText(const std::string &key)
{
	const auto iter = this->templateDat.find(key);
	DebugAssert(iter != this->templateDat.end(), "TEMPLATE.DAT key \"" +
		key + "\" not found.");

	const std::string &value = iter->second;
	return value;
}

const std::vector<CharacterQuestion> &MiscAssets::getQuestionTxtQuestions() const
{
	return this->questionTxt;
}

const CharacterClassGeneration &MiscAssets::getClassGenData() const
{
	return this->classesDat;
}

const std::vector<CharacterClass> &MiscAssets::getClassDefinitions() const
{
	return this->classDefinitions;
}

const std::vector<std::pair<std::string, std::string>> &MiscAssets::getDungeonTxtDungeons() const
{
	return this->dungeonTxt;
}

const MiscAssets::ArtifactText &MiscAssets::getArtifactText1() const
{
	return this->artifactText1;
}

const MiscAssets::ArtifactText &MiscAssets::getArtifactText2() const
{
	return this->artifactText2;
}

const MiscAssets::TradeText &MiscAssets::getTradeText() const
{
	return this->tradeText;
}

std::string MiscAssets::generateNpcName(int raceID, bool isMale, ArenaRandom &random) const
{
	// Get the rules associated with the race and gender.
	const auto &chunkRules = NameRules.at((raceID * 2) + (isMale ? 0 : 1));

	// Construct the name from each part of the rule.
	std::string name;
	for (const auto &rule : chunkRules)
	{
		if (rule.type == NameRule::Type::Index)
		{
			const auto &chunkList = this->nameChunks.at(rule.index);
			name += chunkList.at(random.next() % static_cast<int>(chunkList.size()));
		}
		else if (rule.type == NameRule::Type::String)
		{
			name += std::string(rule.str.data());
		}
		else if (rule.type == NameRule::Type::IndexChance)
		{
			const auto &chunkList = this->nameChunks.at(rule.indexChance.index);
			if ((random.next() % 100) <= rule.indexChance.chance)
			{
				name += chunkList.at(random.next() % static_cast<int>(chunkList.size()));
			}
		}
		else if (rule.type == NameRule::Type::IndexStringChance)
		{
			const auto &chunkList = this->nameChunks.at(rule.indexStringChance.index);
			if ((random.next() % 100) <= rule.indexStringChance.chance)
			{
				name += chunkList.at(random.next() % static_cast<int>(chunkList.size())) +
					std::string(rule.indexStringChance.str.data());
			}
		}
		else
		{
			throw std::runtime_error("Bad rule type \"" +
				std::to_string(static_cast<int>(rule.type)) + "\".");
		}
	}

	return name;
}

const CityDataFile &MiscAssets::getCityDataFile() const
{
	return this->cityDataFile;
}

const std::array<WorldMapMask, 10> &MiscAssets::getWorldMapMasks() const
{
	return this->worldMapMasks;
}
