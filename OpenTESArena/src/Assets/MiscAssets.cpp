#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <sstream>

#include "ExeStrings.h"
#include "ExeUnpacker.h"
#include "MiscAssets.h"
#include "../Entities/CharacterClassCategoryName.h"
#include "../Items/ArmorMaterialType.h"
#include "../Items/ShieldType.h"
#include "../Items/WeaponType.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/Platform.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

const std::string MiscAssets::AExeKeyValuesMapPath = "data/text/aExeStrings.txt";

MiscAssets::MiscAssets()
	: cityDataFile("CITYDATA.00")
{
	// Decompress A.EXE and place it in a string for later use.
	const ExeUnpacker floppyExe("A.EXE");
	this->aExe = floppyExe.getText();

	// Generate a map of interesting strings from the text of A.EXE.
	this->aExeStrings = std::unique_ptr<ExeStrings>(new ExeStrings(
		this->aExe, Platform::getBasePath() + MiscAssets::AExeKeyValuesMapPath));

	// Read in TEMPLATE.DAT, using "#..." as keys and the text as values.
	this->parseTemplateDat();

	// Read in QUESTION.TXT and create character question objects.
	this->parseQuestionTxt();

	// The start of the .data segment (in A.EXE). Used in determining the positions of
	// allowed shields and weapons arrays.
	const int dataSegmentOffset = 0x32560;

	// Read in CLASSES.DAT.
	assert(this->aExeStrings.get() != nullptr);
	this->parseClasses(this->aExe, *this->aExeStrings.get(), dataSegmentOffset);

	// Read in DUNGEON.TXT and pair each dungeon name with its description.
	this->parseDungeonTxt();

	// Read in the world map mask data from TAMRIEL.MNU.
	this->parseWorldMapMasks();
}

MiscAssets::~MiscAssets()
{

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

void MiscAssets::parseClasses(const std::string &exeText, const ExeStrings &exeStrings,
	int dataSegmentOffset)
{
	const std::string filename = "CLASSES.DAT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Initialize the class generation data with zeroes.
	auto &classes = this->classesDat.classes;
	auto &choices = this->classesDat.choices;
	std::memset(classes.data(), 0, classes.size() * sizeof(classes.front()));
	std::memset(choices.data(), 0, choices.size() * sizeof(choices.front()));

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
	const auto &classNameStrs = exeStrings.getList(ExeStringKey::CharacterClassNames);
	const auto &allowedArmorsStrs = exeStrings.getList(ExeStringKey::AllowedArmors);
	const auto &allowedShieldsStrs = exeStrings.getList(ExeStringKey::AllowedShields);
	const auto &allowedWeaponsStrs = exeStrings.getList(ExeStringKey::AllowedWeapons);
	const auto &preferredAttributesStrs = exeStrings.getList(ExeStringKey::ClassAttributes);
	const auto &classNumbersToIDsStrs = exeStrings.getList(ExeStringKey::ClassNumberToClassID);
	const auto &classInitialExpCapStrs = exeStrings.getList(ExeStringKey::ClassInitialExperienceCap);
	const auto &healthDieStrs = exeStrings.getList(ExeStringKey::HealthDice);
	const auto &lockpickingDivisorStrs = exeStrings.getList(ExeStringKey::LockpickingDivisors);

	const int classCount = 18;
	for (int i = 0; i < classCount; i++)
	{
		const std::string &name = classNameStrs.at(i);
		const std::string &preferredAttributes = preferredAttributesStrs.at(i);

		const std::vector<ArmorMaterialType> allowedArmors = [&allowedArmorsStrs, i]()
		{
			// Determine which armors are allowed based on a one-digit value.
			const std::string &valueStr = allowedArmorsStrs.at(i);
			const uint8_t value = static_cast<uint8_t>(valueStr.at(0));

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

		const std::vector<ShieldType> allowedShields = [&exeText,
			dataSegmentOffset, &allowedShieldsStrs, i]()
		{
			// Use the pointer offset at the 'i' index of the shield pointers to find which
			// ID array to use.
			const std::string &offsetStr = allowedShieldsStrs.at(i);
			const uint16_t offset = Bytes::getLE16(
				reinterpret_cast<const uint8_t*>(offsetStr.data()));

			// If the pointer offset is "null", that means all shields are allowed for this class.
			// Otherwise, read each byte in the array until a 0xFF byte.
			if (offset == 0)
			{
				return std::vector<ShieldType>
				{
					ShieldType::Buckler, ShieldType::Round, ShieldType::Kite, ShieldType::Tower
				};
			}
			else
			{
				// Start and end of the 0xFF-terminated array in the executable.
				const int arrayStart = dataSegmentOffset + offset;
				const uint8_t endByte = 0xFF;

				int index = 0;
				std::vector<ShieldType> shields;

				// Read shield IDs until the end byte.
				while (true)
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

					const uint8_t shieldID = static_cast<uint8_t>(exeText.at(arrayStart + index));

					if (shieldID == endByte)
					{
						break;
					}
					else
					{
						shields.push_back(ShieldIDMappings.at(shieldID - 7));
					}

					index++;
				}

				return shields;
			}
		}();

		const std::vector<WeaponType> allowedWeapons = [&exeText,
			dataSegmentOffset, &allowedWeaponsStrs, i]()
		{
			// Use the pointer offset at the 'i' index of the weapon pointers to find which
			// ID array to use.
			const std::string &offsetStr = allowedWeaponsStrs.at(i);
			const uint16_t offset = Bytes::getLE16(
				reinterpret_cast<const uint8_t*>(offsetStr.data()));

			// If the pointer offset is "null", that means all weapons are allowed for this class.
			// Otherwise, read each byte in the array until a 0xFF byte.
			if (offset == 0)
			{
				return std::vector<WeaponType>
				{
					WeaponType::BattleAxe, WeaponType::Broadsword, WeaponType::Claymore,
					WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Flail,
					WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
					WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow,
					WeaponType::Shortsword, WeaponType::Staff, WeaponType::Tanto,
					WeaponType::Wakizashi, WeaponType::WarAxe, WeaponType::Warhammer
				};
			}
			else
			{
				// Start and end of the 0xFF-terminated array in the executable.
				const int arrayStart = dataSegmentOffset + offset;
				const uint8_t endByte = 0xFF;

				int index = 0;
				std::vector<WeaponType> weapons;

				// Read weapon IDs until the end byte.
				while (true)
				{
					// Mappings of weapon IDs to weapon types, ordered as they are shown in 
					// the executable.
					const std::array<WeaponType, 18> WeaponIDMappings =
					{
						WeaponType::Staff, WeaponType::Dagger, WeaponType::Shortsword,
						WeaponType::Broadsword, WeaponType::Saber, WeaponType::Longsword,
						WeaponType::Claymore, WeaponType::Tanto, WeaponType::Wakizashi,
						WeaponType::Katana, WeaponType::DaiKatana, WeaponType::Mace,
						WeaponType::Flail, WeaponType::Warhammer, WeaponType::WarAxe,
						WeaponType::BattleAxe, WeaponType::ShortBow, WeaponType::LongBow
					};

					const uint8_t weaponID = static_cast<uint8_t>(exeText.at(arrayStart + index));

					if (weaponID == endByte)
					{
						break;
					}
					else
					{
						weapons.push_back(WeaponIDMappings.at(weaponID));
					}

					index++;
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

		const double lockpicking = [&lockpickingDivisorStrs, i]()
		{
			const std::string &divisorStr = lockpickingDivisorStrs.at(i);
			const uint8_t divisor = divisorStr.at(0);
			return static_cast<double>(200 / divisor) / 100.0;
		}();

		const int healthDie = [&healthDieStrs, i]()
		{
			const std::string &dieStr = healthDieStrs.at(i);
			return static_cast<uint8_t>(dieStr.at(0));
		}();

		const int initialExperienceCap = [&classInitialExpCapStrs, i]()
		{
			const std::string &capStr = classInitialExpCapStrs.at(i);
			return Bytes::getLE16(reinterpret_cast<const uint8_t*>(capStr.data()));
		}();

		const int classNumberToID = [&classNumbersToIDsStrs, i]()
		{
			const std::string &numberStr = classNumbersToIDsStrs.at(i);
			return static_cast<uint8_t>(numberStr.at(0));
		}();

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

const ExeStrings &MiscAssets::getAExeStrings() const
{
	return *this->aExeStrings.get();
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

const CityDataFile &MiscAssets::getCityDataFile() const
{
	return this->cityDataFile;
}

const std::array<WorldMapMask, 10> &MiscAssets::getWorldMapMasks() const
{
	return this->worldMapMasks;
}
