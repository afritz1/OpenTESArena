#include <cassert>
#include <cstring>
#include <sstream>

#include "ExeStrings.h"
#include "../Utilities/Debug.h"
#include "../Utilities/KeyValueMap.h"
#include "../Utilities/String.h"

namespace
{
	// Mappings of ExeStringKey enums to keys in the executable's key-value file.
	const std::unordered_map<ExeStringKey, std::string> ExeKeyValueMapKeys =
	{
		{ ExeStringKey::ChooseClassCreation, "ChooseClassCreation" },
		{ ExeStringKey::ChooseClassCreationGenerate, "ChooseClassCreationGenerate" },
		{ ExeStringKey::ChooseClassCreationSelect, "ChooseClassCreationSelect" },
		{ ExeStringKey::ClassQuestionsIntro, "ClassQuestionsIntro" },
		{ ExeStringKey::SuggestedRace, "SuggestedRace" },
		{ ExeStringKey::ChooseClassList, "ChooseClassList" },
		{ ExeStringKey::ChooseName, "ChooseName" },
		{ ExeStringKey::ChooseGender, "ChooseGender" },
		{ ExeStringKey::ChooseGenderMale, "ChooseGenderMale" },
		{ ExeStringKey::ChooseGenderFemale, "ChooseGenderFemale" },
		{ ExeStringKey::ChooseRace, "ChooseRace" },
		{ ExeStringKey::ConfirmRace, "ConfirmRace" },
		{ ExeStringKey::FinalRaceMessage, "FinalRaceMessage" },
		{ ExeStringKey::DistributeClassPoints, "DistributeClassPoints" },

		{ ExeStringKey::CharacterClassNames, "CharacterClassNames" },

		{ ExeStringKey::AllowedArmors, "AllowedArmors" },
		{ ExeStringKey::AllowedShields, "AllowedShields" },
		{ ExeStringKey::AllowedWeapons, "AllowedWeapons" },
		{ ExeStringKey::ClassAttributes, "ClassAttributes" },
		{ ExeStringKey::ClassNumberToClassID, "ClassNumberToClassID" },
		{ ExeStringKey::ClassInitialExperienceCap, "ClassInitialExperienceCap" },
		{ ExeStringKey::HealthDice, "HealthDice" },
		{ ExeStringKey::LockpickingDivisors, "LockpickingDivisors" },

		{ ExeStringKey::CharCreationProvinceNames, "CharCreationProvinceNames" },
		{ ExeStringKey::ProvinceNames, "ProvinceNames" },
		{ ExeStringKey::ProvinceIMGFilenames, "ProvinceIMGFilenames" },

		{ ExeStringKey::RaceNamesSingular, "RaceNamesSingular" },
		{ ExeStringKey::RaceNamesPlural, "RaceNamesPlural" },

		{ ExeStringKey::LogbookIsEmpty, "LogbookIsEmpty" },

		{ ExeStringKey::TimesOfDay, "TimesOfDay" },

		{ ExeStringKey::WeekdayNames, "WeekdayNames" },
		{ ExeStringKey::MonthNames, "MonthNames" },

		{ ExeStringKey::CreatureNames, "CreatureNames" },

		{ ExeStringKey::CreatureAnimations, "CreatureAnimations" },
		{ ExeStringKey::MaleCitizenAnimations, "MaleCitizenAnimations" },
		{ ExeStringKey::FemaleCitizenAnimations, "FemaleCitizenAnimations" },
		{ ExeStringKey::CFAFilenameChunks, "CFAFilenameChunks" },
		{ ExeStringKey::CFAFilenameTemplates, "CFAFilenameTemplates" },
		{ ExeStringKey::CFAHumansWithWeaponAnimations, "CFAHumansWithWeaponAnimations" },
		{ ExeStringKey::CFAWeaponAnimations, "CFAWeaponAnimations" },

		{ ExeStringKey::BodyPartNames, "BodyPartNames" },

		{ ExeStringKey::ArmorNames, "ArmorNames", },
		{ ExeStringKey::ShieldNames, "ShieldNames", },
		{ ExeStringKey::WeaponNames, "WeaponNames", }
	};
}

ExeStrings::ExeStrings(const std::string &exeText, const std::string &keyValueMapFilename)
{
	// Load offset and size string pairs into a key-value map.
	const KeyValueMap keyValueMap(keyValueMapFilename);

	// Retrieve each key-value pair, decide if the value is a single pair or a list 
	// of pairs, and insert the corresponding executable string(s) into the proper map.
	for (const auto &pair : ExeKeyValueMapKeys)
	{
		// Separator characters for lists and pairs.
		const char LIST_SEPARATOR = ';';
		const char PAIR_SEPARATOR = ',';

		// Get the value string from the key-value map, and sanitize it a bit. No need
		// to remove carriage returns, since that's done by the key-value map.
		const std::string value = [&keyValueMap, &pair]()
		{
			std::string str = keyValueMap.getString(pair.second);
			str = String::trim(str);
			return str;
		}();

		// Split the value string on semicolons.
		const std::vector<std::string> valuesList = String::split(value, LIST_SEPARATOR);

		// Lambda for constructing a string from some offset + size pair.
		auto makeExeString = [&exeText](const std::vector<std::string> &pairElements)
		{
			// Lambda for converting a hex string to an integer.
			auto hexStringToInt = [](const std::string &str)
			{
				int value;
				std::stringstream ss;
				ss << std::hex << str;
				ss >> value;
				return value;
			};

			// Offset in the executable.
			const int offset = hexStringToInt(pairElements.at(0));

			// Number of bytes to read from the offset.
			const int size = [&exeText, &pairElements, offset]()
			{
				const std::string &element = pairElements.at(1);

				// If the element is a dash, it's a null-terminated string.
				// Otherwise, it's a number.
				return (element == "-") ? 
					static_cast<int>(std::strlen(exeText.data() + offset)) :
					std::stoi(element);
			}();

			// Get the substring from the executable text.
			return exeText.substr(offset, size);
		};

		// If there's only one element, then it's a single pair. Otherwise, it's a list
		// of pairs.
		if (valuesList.size() == 1)
		{
			// Separate the offset and size.
			const std::vector<std::string> pairElements =
				String::split(valuesList.front(), PAIR_SEPARATOR);

			DebugAssert(pairElements.size() == 2, "Invalid element count for \"" + 
				pair.second + "\" in " + keyValueMapFilename + ".");

			// Get the substring from the executable text.
			const std::string exeString = makeExeString(pairElements);

			// Insert the pair into the single element strings map.
			this->strings.insert(std::make_pair(pair.first, exeString));
		}
		else
		{
			// Insert an empty vector into the string lists.
			const auto listIter = this->stringLists.insert(
				std::make_pair(pair.first, std::vector<std::string>())).first;

			// Iterate over each pair in the list, inserting strings into the list.
			for (size_t i = 0; i < valuesList.size(); i++)
			{
				const std::string &valuePair = valuesList.at(i);

				// Separate the offset and size.
				const std::vector<std::string> pairElements =
					String::split(valuePair, PAIR_SEPARATOR);

				DebugAssert(pairElements.size() == 2, "Invalid element count for \"" +
					pair.second + "\" (index " + std::to_string(i) + ") in " + 
					keyValueMapFilename + ".");
				
				// Get the substring from the executable text.
				const std::string exeString = makeExeString(pairElements);

				// Insert the string into the string list.
				listIter->second.push_back(exeString);
			}
		}
	}
}

ExeStrings::~ExeStrings()
{

}

const std::string &ExeStrings::get(ExeStringKey key) const
{
	const auto iter = this->strings.find(key);
	assert(iter != this->strings.end());
	return iter->second;
}

const std::vector<std::string> &ExeStrings::getList(ExeStringKey key) const
{
	const auto iter = this->stringLists.find(key);
	assert(iter != this->stringLists.end());
	return iter->second;
}
