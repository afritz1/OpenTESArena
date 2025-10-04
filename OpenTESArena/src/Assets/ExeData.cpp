#include <algorithm>
#include <cstring>
#include <sstream>

#include "ExeData.h"
#include "ExeUnpacker.h"
#include "../Utilities/Platform.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/KeyValueFile.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace
{
	static constexpr char PAIR_SEPARATOR = ',';

	template<typename T, size_t Length>
	void initInt8Array(T (&arr)[Length], Span<const std::byte> exeBytes, int exeAddress)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < std::size(arr); i++)
		{
			arr[i] = static_cast<T>(exeBytes[exeAddress + i]);
		}
	}

	template<typename T, size_t Length>
	void initInt8PairArray(std::pair<T, T> (&arr)[Length], Span<const std::byte> exeBytes, int exeAddress)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < std::size(arr); i++)
		{
			const size_t index = i * 2;

			std::pair<T, T> &pair = arr[i];
			pair.first = static_cast<T>(exeBytes[exeAddress + index]);
			pair.second = static_cast<T>(exeBytes[exeAddress + index + 1]);
		}
	}

	template<typename T, size_t Length>
	void initJaggedInt8Array(std::vector<T> (&arr)[Length], T terminator, Span<const std::byte> exeBytes, int exeAddress)
	{
		static_assert(sizeof(T) == 1);

		size_t offset = 0;
		for (std::vector<T> &vec : arr)
		{
			const char *innerData = reinterpret_cast<const char*>(exeBytes.begin()) + exeAddress + offset;
			size_t innerOffset = 0;

			while (true)
			{
				const T value = static_cast<T>(*(innerData + innerOffset));

				if (value != terminator)
				{
					vec.emplace_back(value);
				}
				else
				{
					break;
				}

				innerOffset++;
			}

			offset += vec.size() + 1;
		}
	}

	template<typename T, size_t Rows, size_t Columns>
	void init2DInt8Array(T (&arrs)[Columns][Rows], Span<const std::byte> exeBytes, int exeAddress)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < std::size(arrs); i++)
		{
			T (&arr)[Rows] = arrs[i];

			for (size_t j = 0; j < std::size(arr); j++)
			{
				arr[j] = static_cast<T>(exeBytes[exeAddress + (i * Rows) + j]);
			}
		}
	}

	template<typename T, size_t Length>
	void initInt16Array(T (&arr)[Length], Span<const std::byte> exeBytes, int exeAddress)
	{
		static_assert(sizeof(T) == 2);
		const uint8_t *ptr = reinterpret_cast<const uint8_t*>(exeBytes.begin()) + exeAddress;

		for (size_t i = 0; i < std::size(arr); i++)
		{
			arr[i] = static_cast<T>(Bytes::getLE16(ptr + (i * 2)));
		}
	}

	template<typename T, size_t Length>
	void initInt16PairArray(std::pair<T, T> (&arr)[Length], Span<const std::byte> exeBytes, int exeAddress)
	{
		static_assert(sizeof(T) == 2);
		const uint8_t *ptr = reinterpret_cast<const uint8_t*>(exeBytes.begin()) + exeAddress;

		for (size_t i = 0; i < std::size(arr); i++)
		{
			std::pair<T, T> &pair = arr[i];
			pair.first = static_cast<T>(Bytes::getLE16(ptr + (i * 4)));
			pair.second = static_cast<T>(Bytes::getLE16(ptr + ((i * 4) + 2)));
		}
	}

	template<typename T, size_t Length>
	void initInt32Array(T (&arr)[Length], Span<const std::byte> exeBytes, int exeAddress)
	{
		static_assert(sizeof(T) == 4);
		const uint8_t *ptr = reinterpret_cast<const uint8_t*>(exeBytes.begin()) + exeAddress;

		for (size_t i = 0; i < std::size(arr); i++)
		{
			arr[i] = static_cast<T>(Bytes::getLE32(ptr + (i * 4)));
		}
	}

	template<typename T, size_t Length>
	void initIndexArray(int (&indexArr)[Length], const T (&arr)[Length])
	{
		// Construct an array of unique, sorted offsets based on the input array.
		T uniqueArr[Length];
		const auto uniqueBegin = std::begin(uniqueArr);

		// Ignore zeroes because they count as null instead.
		const auto removeIter = std::remove_copy(std::begin(arr), std::end(arr), uniqueBegin, 0);
		std::sort(uniqueBegin, removeIter);
		const auto uniqueEnd = std::unique(uniqueBegin, removeIter);

		for (size_t i = 0; i < std::size(arr); i++)
		{
			const T offset = arr[i];

			int index = -1; // No restrictions by default
			if (offset != 0)
			{
				// Find the position of the offset in the unique offsets array.
				const auto offsetIter = std::find(uniqueBegin, uniqueEnd, offset);
				index = static_cast<int>(std::distance(uniqueBegin, offsetIter));
			}

			indexArr[i] = index;
		}
	}

	template<size_t T>
	void initStringArrayNullTerminated(std::string (&arr)[T], Span<const std::byte> exeBytes, int exeAddress)
	{
		size_t currentStrOffset = 0;
		for (std::string &str : arr)
		{
			const char *currentStrStart = reinterpret_cast<const char*>(exeBytes.begin()) + exeAddress + currentStrOffset;
			const int currentStrLength = static_cast<int>(std::strlen(currentStrStart));
			DebugAssert(exeBytes.isValidRange(exeAddress + currentStrOffset, currentStrLength));
			str = std::string(currentStrStart, currentStrLength);
			currentStrOffset += str.size() + 1;
		}
	}

	int GetExeAddress(const KeyValueFileSection &section, const std::string &key)
	{
		std::string_view valueStr;
		if (!section.tryGetString(key, valueStr))
		{
			DebugLogErrorFormat("Couldn't get \"%s\" (section \"%s\").", key.c_str(), section.getName().c_str());
			return 0;
		}

		// Make sure the value only has an offset and isn't an offset + length pair.
		DebugAssertMsgFormat(valueStr.find(PAIR_SEPARATOR) == std::string_view::npos, "\"%s\" (section \"%s\") should only have an offset.", key.c_str(), section.getName().c_str());

		int offset;

		std::stringstream ss;
		ss << std::hex << valueStr;
		ss >> offset;
		return offset;
	}

	std::pair<int, int> GetExeAddressAndLength(const KeyValueFileSection &section, const std::string &key)
	{
		std::string_view valueStr;
		if (!section.tryGetString(key, valueStr))
		{
			DebugLogErrorFormat("Couldn't get \"%s\" (section \"%s\").", key.c_str(), section.getName().c_str());
			return std::make_pair(0, 0);
		}

		// Make sure the value has a comma-separated offset + length pair.
		std::array<std::string_view, 2> tokens;
		if (!StringView::splitExpected<2>(valueStr, PAIR_SEPARATOR, tokens))
		{
			DebugCrashFormat("Invalid offset + length pair \"%s\" (section \"%s\").", key.c_str(), section.getName().c_str());
		}

		const std::string_view offsetStr = tokens[0];
		const std::string_view lengthStr = tokens[1];
		int offset, length;

		std::stringstream ss;
		const auto streamFlags = ss.flags();
		ss << std::hex << offsetStr;
		ss >> offset;
		ss.clear();
		ss.flags(streamFlags);
		ss << lengthStr;
		ss >> length;
		return std::make_pair(offset, length);
	}

	std::string GetExeStringNullTerminated(Span<const std::byte> exeBytes, int exeAddress)
	{
		const char *strBegin = reinterpret_cast<const char*>(exeBytes.begin()) + exeAddress;
		const int length = static_cast<int>(std::strlen(strBegin));
		DebugAssert(exeBytes.isValidRange(exeAddress, length));
		return std::string(strBegin, length);
	}

	std::string GetExeStringFixedLength(Span<const std::byte> exeBytes, const std::pair<int, int> &offsetAndLength)
	{
		const int exeAddress = offsetAndLength.first;
		const int length = offsetAndLength.second;
		DebugAssert(exeBytes.isValidRange(exeAddress, length));
		return std::string(reinterpret_cast<const char*>(exeBytes.begin()) + exeAddress, length);
	}
}

bool ExeDataCalendar::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Calendar";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int monthNamesOffset = GetExeAddress(*section, "MonthNames");
	const int timesOfDayOffset = GetExeAddress(*section, "TimesOfDay");
	const int weekdayNamesOffset = GetExeAddress(*section, "WeekdayNames");
	const int holidayNamesOffset = GetExeAddress(*section, "HolidayNames");
	const int holidayDatesOffset = GetExeAddress(*section, "HolidayDates");

	initStringArrayNullTerminated(this->monthNames, exeBytes, monthNamesOffset);
	initStringArrayNullTerminated(this->timesOfDay, exeBytes, timesOfDayOffset);
	initStringArrayNullTerminated(this->weekdayNames, exeBytes, weekdayNamesOffset);
	initStringArrayNullTerminated(this->holidayNames, exeBytes, holidayNamesOffset);
	initInt16Array(this->holidayDates, exeBytes, holidayDatesOffset);

	return true;
}

bool ExeDataCharacterClasses::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "CharacterClasses";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int allowedArmorsOffset = GetExeAddress(*section, "AllowedArmors");
	const int allowedShieldsOffset = GetExeAddress(*section, "AllowedShields");
	const int allowedShieldsListsOffset = GetExeAddress(*section, "AllowedShieldsLists");
	const int allowedWeaponsOffset = GetExeAddress(*section, "AllowedWeapons");
	const int allowedWeaponsListsOffset = GetExeAddress(*section, "AllowedWeaponsLists");
	const int classNamesOffset = GetExeAddress(*section, "ClassNames");
	const int classNumbersToIDsOffset = GetExeAddress(*section, "ClassNumbersToIDs");
	const int healthDiceOffset = GetExeAddress(*section, "HealthDice");
	const int initialExpCapsOffset = GetExeAddress(*section, "InitialExperienceCaps");
	const int thievingDivisorsOffset = GetExeAddress(*section, "ThievingDivisors");
	const int preferredAttributesOffset = GetExeAddress(*section, "PreferredAttributes");
	const int magicClassIntelligenceMultipliersOffset = GetExeAddress(*section, "MagicClassIntelligenceMultipliers");

	initInt8Array(this->allowedArmors, exeBytes, allowedArmorsOffset);
	initInt16Array(this->allowedShields, exeBytes, allowedShieldsOffset);

	const uint8_t shieldTerminator = 0xFF;
	initJaggedInt8Array(this->allowedShieldsLists, shieldTerminator, exeBytes, allowedShieldsListsOffset);

	initInt16Array(this->allowedWeapons, exeBytes, allowedWeaponsOffset);

	const uint8_t weaponTerminator = 0xFF;
	initJaggedInt8Array(this->allowedWeaponsLists, weaponTerminator, exeBytes, allowedWeaponsListsOffset);

	initIndexArray(this->allowedShieldsIndices, this->allowedShields);
	initIndexArray(this->allowedWeaponsIndices, this->allowedWeapons);
	initStringArrayNullTerminated(this->classNames, exeBytes, classNamesOffset);
	initInt8Array(this->classNumbersToIDs, exeBytes, classNumbersToIDsOffset);
	initInt8Array(this->healthDice, exeBytes, healthDiceOffset);
	initInt16Array(this->initialExperienceCaps, exeBytes, initialExpCapsOffset);
	initInt8Array(this->thievingDivisors, exeBytes, thievingDivisorsOffset);
	initStringArrayNullTerminated(this->preferredAttributes, exeBytes, preferredAttributesOffset);
	initInt8Array(this->magicClassIntelligenceMultipliers, exeBytes, magicClassIntelligenceMultipliersOffset);

	return true;
}

bool ExeDataCharacterCreation::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "CharacterCreation";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const auto chooseClassCreationPair = GetExeAddressAndLength(*section, "ChooseClassCreation");
	const auto chooseClassCreationGeneratePair = GetExeAddressAndLength(*section, "ChooseClassCreationGenerate");
	const auto chooseClassCreationSelectPair = GetExeAddressAndLength(*section, "ChooseClassCreationSelect");
	const auto classQuestionsIntroPair = GetExeAddressAndLength(*section, "ClassQuestionsIntro");
	const auto suggestedClassPair = GetExeAddressAndLength(*section, "SuggestedClass");
	const auto chooseClassListPair = GetExeAddressAndLength(*section, "ChooseClassList");
	const auto chooseNamePair = GetExeAddressAndLength(*section, "ChooseName");
	const auto chooseGenderPair = GetExeAddressAndLength(*section, "ChooseGender");
	const auto chooseGenderMalePair = GetExeAddressAndLength(*section, "ChooseGenderMale");
	const auto chooseGenderFemalePair = GetExeAddressAndLength(*section, "ChooseGenderFemale");
	const auto chooseRacePair = GetExeAddressAndLength(*section, "ChooseRace");
	const auto confirmRacePair = GetExeAddressAndLength(*section, "ConfirmRace");
	const auto confirmedRace1Pair = GetExeAddressAndLength(*section, "ConfirmedRace1");
	const auto confirmedRace2Pair = GetExeAddressAndLength(*section, "ConfirmedRace2");
	const auto confirmedRace3Pair = GetExeAddressAndLength(*section, "ConfirmedRace3");
	const auto confirmedRace4Pair = GetExeAddressAndLength(*section, "ConfirmedRace4");
	const auto distributeClassPointsPair = GetExeAddressAndLength(*section, "DistributeClassPoints");
	const auto chooseAttributesPair = GetExeAddressAndLength(*section, "ChooseAttributes");
	const auto chooseAttributesSavePair = GetExeAddressAndLength(*section, "ChooseAttributesSave");
	const auto chooseAttributesRerollPair = GetExeAddressAndLength(*section, "ChooseAttributesReroll");
	const auto chooseAttributesBonusPointsRemainingOffset = GetExeAddress(*section, "ChooseAttributesBonusPointsRemaining");
	const auto chooseAppearancePair = GetExeAddressAndLength(*section, "ChooseAppearance");

	this->chooseClassCreation = GetExeStringFixedLength(exeBytes, chooseClassCreationPair);
	this->chooseClassCreationGenerate = GetExeStringFixedLength(exeBytes, chooseClassCreationGeneratePair);
	this->chooseClassCreationSelect = GetExeStringFixedLength(exeBytes, chooseClassCreationSelectPair);
	this->classQuestionsIntro = GetExeStringFixedLength(exeBytes, classQuestionsIntroPair);
	this->suggestedClass = GetExeStringFixedLength(exeBytes, suggestedClassPair);
	this->chooseClassList = GetExeStringFixedLength(exeBytes, chooseClassListPair);
	this->chooseName = GetExeStringFixedLength(exeBytes, chooseNamePair);
	this->chooseGender = GetExeStringFixedLength(exeBytes, chooseGenderPair);
	this->chooseGenderMale = GetExeStringFixedLength(exeBytes, chooseGenderMalePair);
	this->chooseGenderFemale = GetExeStringFixedLength(exeBytes, chooseGenderFemalePair);
	this->chooseRace = GetExeStringFixedLength(exeBytes, chooseRacePair);
	this->confirmRace = GetExeStringFixedLength(exeBytes, confirmRacePair);
	this->confirmedRace1 = GetExeStringFixedLength(exeBytes, confirmedRace1Pair);
	this->confirmedRace2 = GetExeStringFixedLength(exeBytes, confirmedRace2Pair);
	this->confirmedRace3 = GetExeStringFixedLength(exeBytes, confirmedRace3Pair);
	this->confirmedRace4 = GetExeStringFixedLength(exeBytes, confirmedRace4Pair);
	this->distributeClassPoints = GetExeStringFixedLength(exeBytes, distributeClassPointsPair);
	this->chooseAttributes = GetExeStringFixedLength(exeBytes, chooseAttributesPair);
	this->chooseAttributesSave = GetExeStringFixedLength(exeBytes, chooseAttributesSavePair);
	this->chooseAttributesReroll = GetExeStringFixedLength(exeBytes, chooseAttributesRerollPair);
	this->chooseAttributesBonusPointsRemaining = GetExeStringNullTerminated(exeBytes, chooseAttributesBonusPointsRemainingOffset);
	this->chooseAppearance = GetExeStringFixedLength(exeBytes, chooseAppearancePair);

	return true;
}

bool ExeDataCityGeneration::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "CityGeneration";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int coastalCityListOffset = GetExeAddress(*section, "CoastalCityList");
	const int cityTemplateFilenamesOffset = GetExeAddress(*section, "CityTemplateFilenames");
	const int startingPositionsOffset = GetExeAddress(*section, "StartingPositions");
	const int reservedBlockListsOffset = GetExeAddress(*section, "ReservedBlockLists");
	const int tavernPrefixesOffset = GetExeAddress(*section, "TavernPrefixes");
	const int tavernMarineSuffixesOffset = GetExeAddress(*section, "TavernMarineSuffixes");
	const int tavernSuffixesOffset = GetExeAddress(*section, "TavernSuffixes");
	const int templePrefixesOffset = GetExeAddress(*section, "TemplePrefixes");
	const int temple1SuffixesOffset = GetExeAddress(*section, "Temple1Suffixes");
	const int temple2SuffixesOffset = GetExeAddress(*section, "Temple2Suffixes");
	const int temple3SuffixesOffset = GetExeAddress(*section, "Temple3Suffixes");
	const int equipmentPrefixesOffset = GetExeAddress(*section, "EquipmentPrefixes");
	const int equipmentSuffixesOffset = GetExeAddress(*section, "EquipmentSuffixes");
	const int magesGuildMenuNameOffset = GetExeAddress(*section, "MagesGuildMenuName");

	initInt8Array(this->coastalCityList, exeBytes, coastalCityListOffset);
	initStringArrayNullTerminated(this->templateFilenames, exeBytes, cityTemplateFilenamesOffset);
	initInt8PairArray(this->startingPositions, exeBytes, startingPositionsOffset);

	const uint8_t blockTerminator = 0;
	initJaggedInt8Array(this->reservedBlockLists, blockTerminator, exeBytes, reservedBlockListsOffset);

	initStringArrayNullTerminated(this->tavernPrefixes, exeBytes, tavernPrefixesOffset);
	initStringArrayNullTerminated(this->tavernMarineSuffixes, exeBytes, tavernMarineSuffixesOffset);
	initStringArrayNullTerminated(this->tavernSuffixes, exeBytes, tavernSuffixesOffset);
	initStringArrayNullTerminated(this->templePrefixes, exeBytes, templePrefixesOffset);
	initStringArrayNullTerminated(this->temple1Suffixes, exeBytes, temple1SuffixesOffset);
	initStringArrayNullTerminated(this->temple2Suffixes, exeBytes, temple2SuffixesOffset);
	initStringArrayNullTerminated(this->temple3Suffixes, exeBytes, temple3SuffixesOffset);
	initStringArrayNullTerminated(this->equipmentPrefixes, exeBytes, equipmentPrefixesOffset);
	initStringArrayNullTerminated(this->equipmentSuffixes, exeBytes, equipmentSuffixesOffset);
	this->magesGuildMenuName = GetExeStringNullTerminated(exeBytes, magesGuildMenuNameOffset);

	return true;
}

bool ExeDataEntities::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Entities";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int attributeNamesOffset = GetExeAddress(*section, "AttributeNames");
	const int creatureNamesOffset = GetExeAddress(*section, "CreatureNames");
	const int creatureLevelsOffset = GetExeAddress(*section, "CreatureLevels");
	const int creatureHitPointsOffset = GetExeAddress(*section, "CreatureHitPoints");
	const int creatureBaseExpsOffset = GetExeAddress(*section, "CreatureBaseExperience");
	const int creatureExpMultipliersOffset = GetExeAddress(*section, "CreatureExperienceMultipliers");
	const int creatureSoundsOffset = GetExeAddress(*section, "CreatureSounds");
	const int creatureSoundNamesOffset = GetExeAddress(*section, "CreatureSoundNames");
	const int creatureDamagesOffset = GetExeAddress(*section, "CreatureDamages");
	const int creatureMagicEffectsOffset = GetExeAddress(*section, "CreatureMagicEffects");
	const int creatureScalesOffset = GetExeAddress(*section, "CreatureScales");
	const int creatureYOffsetsOffset = GetExeAddress(*section, "CreatureYOffsets");
	const int creatureHasNoCorpseOffset = GetExeAddress(*section, "CreatureHasNoCorpse");
	const int creatureBloodOffset = GetExeAddress(*section, "CreatureBlood");
	const int creatureDiseaseChancesOffset = GetExeAddress(*section, "CreatureDiseaseChances");
	const int creatureAttributesOffset = GetExeAddress(*section, "CreatureAttributes");
	const int creatureLootChancesOffset = GetExeAddress(*section, "CreatureLootChances");
	const int creatureAnimFilenamesOffset = GetExeAddress(*section, "CreatureAnimationFilenames");
	const int finalBossNameOffset = GetExeAddress(*section, "FinalBossName");
	const int humanEnemyGoldChancesOffset = GetExeAddress(*section, "HumanEnemyGoldChances");
	const int raceAttributesOffset = GetExeAddress(*section, "RaceAttributes");
	const int guardAttributesOffset = GetExeAddress(*section, "GuardAttributes");
	const int maleCitizenAnimFilenamesOffset = GetExeAddress(*section, "MaleCitizenAnimationFilenames");
	const int femaleCitizenAnimFilenamesOffset = GetExeAddress(*section, "FemaleCitizenAnimationFilenames");
	const int humanFilenameTypesOffset = GetExeAddress(*section, "HumanFilenameTypes");
	const int humanFilenameTemplatesOffset = GetExeAddress(*section, "HumanFilenameTemplates");
	const int cfaHumansWithWeaponAnimsOffset = GetExeAddress(*section, "CFAHumansWithWeaponAnimations");
	const int cfaWeaponAnimationsOffset = GetExeAddress(*section, "CFAWeaponAnimations");
	const int effectAnimsOffset = GetExeAddress(*section, "EffectAnimations");
	const int citizenColorBaseOffset = GetExeAddress(*section, "CitizenColorBase");
	const int citizenSkinColorsOffset = GetExeAddress(*section, "CitizenSkinColors");

	initStringArrayNullTerminated(this->attributeNames, exeBytes, attributeNamesOffset);
	initStringArrayNullTerminated(this->creatureNames, exeBytes, creatureNamesOffset);
	initInt8Array(this->creatureLevels, exeBytes, creatureLevelsOffset);
	initInt16PairArray(this->creatureHitPoints, exeBytes, creatureHitPointsOffset);
	initInt32Array(this->creatureBaseExps, exeBytes, creatureBaseExpsOffset);
	initInt8Array(this->creatureExpMultipliers, exeBytes, creatureExpMultipliersOffset);
	initInt8Array(this->creatureSounds, exeBytes, creatureSoundsOffset);
	initStringArrayNullTerminated(this->creatureSoundNames, exeBytes, creatureSoundNamesOffset);
	initInt8PairArray(this->creatureDamages, exeBytes, creatureDamagesOffset);
	initInt16Array(this->creatureMagicEffects, exeBytes, creatureMagicEffectsOffset);
	initInt16Array(this->creatureScales, exeBytes, creatureScalesOffset);
	initInt8Array(this->creatureYOffsets, exeBytes, creatureYOffsetsOffset);
	initInt8Array(this->creatureHasNoCorpse, exeBytes, creatureHasNoCorpseOffset);
	initInt8Array(this->creatureBlood, exeBytes, creatureBloodOffset);
	initInt8Array(this->creatureDiseaseChances, exeBytes, creatureDiseaseChancesOffset);
	init2DInt8Array(this->creatureAttributes, exeBytes, creatureAttributesOffset);
	initInt32Array(this->creatureLootChances, exeBytes, creatureLootChancesOffset);
	initStringArrayNullTerminated(this->creatureAnimationFilenames, exeBytes, creatureAnimFilenamesOffset);
	this->finalBossName = GetExeStringNullTerminated(exeBytes, finalBossNameOffset);
	initInt8Array(this->humanEnemyGoldChances, exeBytes, humanEnemyGoldChancesOffset);
	init2DInt8Array(this->raceAttributes, exeBytes, raceAttributesOffset);
	init2DInt8Array(this->guardAttributes, exeBytes, guardAttributesOffset);
	initStringArrayNullTerminated(this->maleCitizenAnimationFilenames, exeBytes, maleCitizenAnimFilenamesOffset);
	initStringArrayNullTerminated(this->femaleCitizenAnimationFilenames, exeBytes, femaleCitizenAnimFilenamesOffset);
	initStringArrayNullTerminated(this->humanFilenameTypes, exeBytes, humanFilenameTypesOffset);
	initStringArrayNullTerminated(this->humanFilenameTemplates, exeBytes, humanFilenameTemplatesOffset);
	initStringArrayNullTerminated(this->cfaHumansWithWeaponAnimations, exeBytes, cfaHumansWithWeaponAnimsOffset);
	initStringArrayNullTerminated(this->cfaWeaponAnimations, exeBytes, cfaWeaponAnimationsOffset);
	initStringArrayNullTerminated(this->effectAnimations, exeBytes, effectAnimsOffset);
	initInt8Array(this->citizenColorBase, exeBytes, citizenColorBaseOffset);
	initInt8Array(this->citizenSkinColors, exeBytes, citizenSkinColorsOffset);

	return true;
}

bool ExeDataEquipment::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Equipment";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int enchantmentChancesOffset = GetExeAddress(*section, "EnchantmentChances");
	const int materialNamesOffset = GetExeAddress(*section, "MaterialNames");
	const int materialBonusesOffset = GetExeAddress(*section, "MaterialBonuses");
	const int materialChancesOffset = GetExeAddress(*section, "MaterialChances");
	const int materialPriceMultipliersOffset = GetExeAddress(*section, "MaterialPriceMultipliers");
	const int itemConditionNamesOffset = GetExeAddress(*section, "ItemConditionNames");
	const int itemConditionChancesOffset = GetExeAddress(*section, "ItemConditionChances");
	const int itemConditionPercentagesOffset = GetExeAddress(*section, "ItemConditionPercentages");
	const int armorNamesOffset = GetExeAddress(*section, "ArmorNames");
	const int plateArmorNamesOffset = GetExeAddress(*section, "PlateArmorNames");
	const int plateArmorQualitiesOffset = GetExeAddress(*section, "PlateArmorQualities");
	const int plateArmorBasePricesOffset = GetExeAddress(*section, "PlateArmorBasePrices");
	const int plateArmorWeightsOffset = GetExeAddress(*section, "PlateArmorWeights");
	const int chainArmorNamesOffset = GetExeAddress(*section, "ChainArmorNames");
	const int chainArmorQualitiesOffset = GetExeAddress(*section, "ChainArmorQualities");
	const int chainArmorBasePricesOffset = GetExeAddress(*section, "ChainArmorBasePrices");
	const int chainArmorWeightsOffset = GetExeAddress(*section, "ChainArmorWeights");
	const int leatherArmorNamesOffset = GetExeAddress(*section, "LeatherArmorNames");
	const int leatherArmorQualitiesOffset = GetExeAddress(*section, "LeatherArmorQualities");
	const int leatherArmorBasePricesOffset = GetExeAddress(*section, "LeatherArmorBasePrices");
	const int leatherArmorWeightsOffset = GetExeAddress(*section, "LeatherArmorWeights");
	const int shieldArmorClassesOffset = GetExeAddress(*section, "ShieldArmorClasses");
	const int armorEnchantmentNamesOffset = GetExeAddress(*section, "ArmorEnchantmentNames");
	const int armorEnchantmentQualitiesOffset = GetExeAddress(*section, "ArmorEnchantmentQualities");
	const int armorEnchantmentSpellsOffset = GetExeAddress(*section, "ArmorEnchantmentSpells");
	const int armorEnchantmentBonusPricesOffset = GetExeAddress(*section, "ArmorEnchantmentBonusPrices");
	const int weaponNamesOffset = GetExeAddress(*section, "WeaponNames");
	const int weaponQualitiesOffset = GetExeAddress(*section, "WeaponQualities");
	const int weaponBasePricesOffset = GetExeAddress(*section, "WeaponBasePrices");
	const int weaponWeightsOffset = GetExeAddress(*section, "WeaponWeights");
	const int weaponDamagesOffset = GetExeAddress(*section, "WeaponDamages");
	const int weaponHandednessesOffset = GetExeAddress(*section, "WeaponHandednesses");
	const int weaponEnchantmentNamesOffset = GetExeAddress(*section, "WeaponEnchantmentNames");
	const int weaponEnchantmentQualitiesOffset = GetExeAddress(*section, "WeaponEnchantmentQualities");
	const int weaponEnchantmentSpellsOffset = GetExeAddress(*section, "WeaponEnchantmentSpells");
	const int weaponEnchantmentBonusPricesOffset = GetExeAddress(*section, "WeaponEnchantmentBonusPrices");
	const int spellcastingItemNamesOffset = GetExeAddress(*section, "SpellcastingItemNames");
	const int spellcastingItemCumulativeChancesOffset = GetExeAddress(*section, "SpellcastingItemCumulativeChances");
	const int spellcastingItemBasePricesOffset = GetExeAddress(*section, "SpellcastingItemBasePrices");
	const int spellcastingItemChargeRangesOffset = GetExeAddress(*section, "SpellcastingItemChargeRanges");
	const int spellcastingItemAttackSpellNamesOffset = GetExeAddress(*section, "SpellcastingItemAttackSpellNames");
	const int spellcastingItemAttackSpellQualitiesOffset = GetExeAddress(*section, "SpellcastingItemAttackSpellQualities");
	const int spellcastingItemAttackSpellSpellsOffset = GetExeAddress(*section, "SpellcastingItemAttackSpellSpells");
	const int spellcastingItemAttackSpellPricesPerChargeOffset = GetExeAddress(*section, "SpellcastingItemAttackSpellPricesPerCharge");
	const int spellcastingItemDefensiveSpellNamesOffset = GetExeAddress(*section, "SpellcastingItemDefensiveSpellNames");
	const int spellcastingItemDefensiveSpellQualitiesOffset = GetExeAddress(*section, "SpellcastingItemDefensiveSpellQualities");
	const int spellcastingItemDefensiveSpellSpellsOffset = GetExeAddress(*section, "SpellcastingItemDefensiveSpellSpells");
	const int spellcastingItemDefensiveSpellPricesPerChargeOffset = GetExeAddress(*section, "SpellcastingItemDefensiveSpellPricesPerCharge");
	const int spellcastingItemMiscSpellNamesOffset = GetExeAddress(*section, "SpellcastingItemMiscSpellNames");
	const int spellcastingItemMiscSpellQualitiesOffset = GetExeAddress(*section, "SpellcastingItemMiscSpellQualities");
	const int spellcastingItemMiscSpellSpellsOffset = GetExeAddress(*section, "SpellcastingItemMiscSpellSpells");
	const int spellcastingItemMiscSpellPricesPerChargeOffset = GetExeAddress(*section, "SpellcastingItemMiscSpellPricesPerCharge");
	const int enhancementItemNamesOffset = GetExeAddress(*section, "EnhancementItemNames");
	const int enhancementItemCumulativeChancesOffset = GetExeAddress(*section, "EnhancementItemCumulativeChances");
	const int enhancementItemBasePricesOffset = GetExeAddress(*section, "EnhancementItemBasePrices");
	const int potionNamesOffset = GetExeAddress(*section, "PotionNames");
	const int unidentifiedPotionNameOffset = GetExeAddress(*section, "UnidentifiedPotionName");
	const int bodyPartNamesOffset = GetExeAddress(*section, "BodyPartNames");
	const int weaponAnimFilenamesOffset = GetExeAddress(*section, "WeaponAnimationFilenames");

	initInt8Array(this->enchantmentChances, exeBytes, enchantmentChancesOffset);
	initStringArrayNullTerminated(this->materialNames, exeBytes, materialNamesOffset);
	initInt8Array(this->materialBonuses, exeBytes, materialBonusesOffset);
	initInt8Array(this->materialChances, exeBytes, materialChancesOffset);
	initInt16Array(this->materialPriceMultipliers, exeBytes, materialPriceMultipliersOffset);
	initStringArrayNullTerminated(this->itemConditionNames, exeBytes, itemConditionNamesOffset);
	initInt8Array(this->itemConditionChances, exeBytes, itemConditionChancesOffset);
	initInt8Array(this->itemConditionPercentages, exeBytes, itemConditionPercentagesOffset);
	initStringArrayNullTerminated(this->armorNames, exeBytes, armorNamesOffset);
	initStringArrayNullTerminated(this->plateArmorNames, exeBytes, plateArmorNamesOffset);
	initInt8Array(this->plateArmorQualities, exeBytes, plateArmorQualitiesOffset);
	initInt8Array(this->plateArmorBasePrices, exeBytes, plateArmorBasePricesOffset);
	initInt16Array(this->plateArmorWeights, exeBytes, plateArmorWeightsOffset);
	initStringArrayNullTerminated(this->chainArmorNames, exeBytes, chainArmorNamesOffset);
	initInt8Array(this->chainArmorQualities, exeBytes, chainArmorQualitiesOffset);
	initInt8Array(this->chainArmorBasePrices, exeBytes, chainArmorBasePricesOffset);
	initInt16Array(this->chainArmorWeights, exeBytes, chainArmorWeightsOffset);
	initStringArrayNullTerminated(this->leatherArmorNames, exeBytes, leatherArmorNamesOffset);
	initInt8Array(this->leatherArmorQualities, exeBytes, leatherArmorQualitiesOffset);
	initInt8Array(this->leatherArmorBasePrices, exeBytes, leatherArmorBasePricesOffset);
	initInt16Array(this->leatherArmorWeights, exeBytes, leatherArmorWeightsOffset);
	initInt8Array(this->shieldArmorClasses, exeBytes, shieldArmorClassesOffset);
	initStringArrayNullTerminated(this->armorEnchantmentNames, exeBytes, armorEnchantmentNamesOffset);
	initInt8Array(this->armorEnchantmentQualities, exeBytes, armorEnchantmentQualitiesOffset);
	initInt8Array(this->armorEnchantmentSpells, exeBytes, armorEnchantmentSpellsOffset);
	initInt16Array(this->armorEnchantmentBonusPrices, exeBytes, armorEnchantmentBonusPricesOffset);
	initStringArrayNullTerminated(this->weaponNames, exeBytes, weaponNamesOffset);
	initInt8Array(this->weaponQualities, exeBytes, weaponQualitiesOffset);
	initInt8Array(this->weaponBasePrices, exeBytes, weaponBasePricesOffset);
	initInt16Array(this->weaponWeights, exeBytes, weaponWeightsOffset);
	initInt8PairArray(this->weaponDamages, exeBytes, weaponDamagesOffset);
	initInt8Array(this->weaponHandednesses, exeBytes, weaponHandednessesOffset);
	initStringArrayNullTerminated(this->weaponEnchantmentNames, exeBytes, weaponEnchantmentNamesOffset);
	initInt8Array(this->weaponEnchantmentQualities, exeBytes, weaponEnchantmentQualitiesOffset);
	initInt8Array(this->weaponEnchantmentSpells, exeBytes, weaponEnchantmentSpellsOffset);
	initInt16Array(this->weaponEnchantmentBonusPrices, exeBytes, weaponEnchantmentBonusPricesOffset);
	initStringArrayNullTerminated(this->spellcastingItemNames, exeBytes, spellcastingItemNamesOffset);
	initInt8Array(this->spellcastingItemCumulativeChances, exeBytes, spellcastingItemCumulativeChancesOffset);
	initInt16Array(this->spellcastingItemBasePrices, exeBytes, spellcastingItemBasePricesOffset);
	initInt8PairArray(this->spellcastingItemChargeRanges, exeBytes, spellcastingItemChargeRangesOffset);
	initStringArrayNullTerminated(this->spellcastingItemAttackSpellNames, exeBytes, spellcastingItemAttackSpellNamesOffset);
	initInt8Array(this->spellcastingItemAttackSpellQualities, exeBytes, spellcastingItemAttackSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemAttackSpellSpells, exeBytes, spellcastingItemAttackSpellSpellsOffset);
	initInt16Array(this->spellcastingItemAttackSpellPricesPerCharge, exeBytes, spellcastingItemAttackSpellPricesPerChargeOffset);
	initStringArrayNullTerminated(this->spellcastingItemDefensiveSpellNames, exeBytes, spellcastingItemDefensiveSpellNamesOffset);
	initInt8Array(this->spellcastingItemDefensiveSpellQualities, exeBytes, spellcastingItemDefensiveSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemDefensiveSpellSpells, exeBytes, spellcastingItemDefensiveSpellSpellsOffset);
	initInt16Array(this->spellcastingItemDefensiveSpellPricesPerCharge, exeBytes, spellcastingItemDefensiveSpellPricesPerChargeOffset);
	initStringArrayNullTerminated(this->spellcastingItemMiscSpellNames, exeBytes, spellcastingItemMiscSpellNamesOffset);
	initInt8Array(this->spellcastingItemMiscSpellQualities, exeBytes, spellcastingItemMiscSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemMiscSpellSpells, exeBytes, spellcastingItemMiscSpellSpellsOffset);
	initInt16Array(this->spellcastingItemMiscSpellPricesPerCharge, exeBytes, spellcastingItemMiscSpellPricesPerChargeOffset);
	initStringArrayNullTerminated(this->enhancementItemNames, exeBytes, enhancementItemNamesOffset);
	initInt8Array(this->enhancementItemCumulativeChances, exeBytes, enhancementItemCumulativeChancesOffset);
	initInt16Array(this->enhancementItemBasePrices, exeBytes, enhancementItemBasePricesOffset);
	initStringArrayNullTerminated(this->potionNames, exeBytes, potionNamesOffset);
	this->unidentifiedPotionName = GetExeStringNullTerminated(exeBytes, unidentifiedPotionNameOffset);
	initStringArrayNullTerminated(this->bodyPartNames, exeBytes, bodyPartNamesOffset);
	initStringArrayNullTerminated(this->weaponAnimationFilenames, exeBytes, weaponAnimFilenamesOffset);

	return true;
}

bool ExeDataItems::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Items";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int goldPieceOffset = GetExeAddress(*section, "GoldPiece");
	const int bagOfGoldPiecesOffset = GetExeAddress(*section, "BagOfGoldPieces");
	const int lootChancesOffset = GetExeAddress(*section, "LootChances");
	const int palaceGoldValuesOffset = GetExeAddress(*section, "PalaceGoldValues");

	this->goldPiece = GetExeStringNullTerminated(exeBytes, goldPieceOffset);
	this->bagOfGoldPieces = GetExeStringNullTerminated(exeBytes, bagOfGoldPiecesOffset);
	initInt8Array(this->lootChances, exeBytes, lootChancesOffset);
	initInt16Array(this->palaceGoldValues, exeBytes, palaceGoldValuesOffset);

	return true;
}

bool ExeDataLight::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Light";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int windowTwilightColorsOffset = GetExeAddress(*section, "WindowTwilightColors");
	const int waterTwilightLightLevelsOffset = GetExeAddress(*section, "WaterTwilightLightLevels");

	initInt8Array(this->windowTwilightColors, exeBytes, windowTwilightColorsOffset);
	initInt16Array(this->waterTwilightLightLevels, exeBytes, waterTwilightLightLevelsOffset);

	return true;
}

bool ExeDataLocations::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Locations";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int provinceNamesOffset = GetExeAddress(*section, "ProvinceNames");
	const int charCreationProvinceNamesOffset = GetExeAddress(*section, "CharCreationProvinceNames");
	const int provinceImgFilenamesOffset = GetExeAddress(*section, "ProvinceImgFilenames");
	const int locationTypesOffset = GetExeAddress(*section, "LocationTypes");
	const int menuMifPrefixesOffset = GetExeAddress(*section, "MenuMifPrefixes");
	const int centerProvinceCityMifNameOffset = GetExeAddress(*section, "CenterProvinceCityMifName");
	const int startDungeonNameOffset = GetExeAddress(*section, "StartDungeonName");
	const int startDungeonMifNameOffset = GetExeAddress(*section, "StartDungeonMifName");
	const int finalDungeonMifNameOffset = GetExeAddress(*section, "FinalDungeonMifName");
	const int staffProvincesOffset = GetExeAddress(*section, "StaffProvinces");
	const int climatesOffset = GetExeAddress(*section, "Climates");
	const int weatherTableOffset = GetExeAddress(*section, "WeatherTable");
	const int climateSpeedTablesOffset = GetExeAddress(*section, "ClimateSpeedTables");
	const int weatherSpeedTablesOffset = GetExeAddress(*section, "WeatherSpeedTables");
	const int rulerTitlesOffset = GetExeAddress(*section, "RulerTitles");
	const int distantMountainFilenamesOffset = GetExeAddress(*section, "DistantMountainFilenames");
	const int animDistantMountainFilenamesOffset = GetExeAddress(*section, "AnimDistantMountainFilenames");
	const int cloudFilenameOffset = GetExeAddress(*section, "CloudFilename");
	const int sunFilenameOffset = GetExeAddress(*section, "SunFilename");
	const int moonFilenamesOffset = GetExeAddress(*section, "MoonFilenames");
	const int starFilenameOffset = GetExeAddress(*section, "StarFilename");

	// Each province name is null-terminated and 98 bytes apart.
	for (size_t i = 0; i < std::size(this->provinceNames); i++)
	{
		this->provinceNames[i] = GetExeStringNullTerminated(exeBytes, provinceNamesOffset + (i * 98));
	}

	initStringArrayNullTerminated(this->charCreationProvinceNames, exeBytes, charCreationProvinceNamesOffset);
	initStringArrayNullTerminated(this->provinceImgFilenames, exeBytes, provinceImgFilenamesOffset);
	initStringArrayNullTerminated(this->locationTypes, exeBytes, locationTypesOffset);
	initStringArrayNullTerminated(this->menuMifPrefixes, exeBytes, menuMifPrefixesOffset);
	this->centerProvinceCityMifName = GetExeStringNullTerminated(exeBytes, centerProvinceCityMifNameOffset);
	this->startDungeonName = GetExeStringNullTerminated(exeBytes, startDungeonNameOffset);
	this->startDungeonMifName = GetExeStringNullTerminated(exeBytes, startDungeonMifNameOffset);
	this->finalDungeonMifName = GetExeStringNullTerminated(exeBytes, finalDungeonMifNameOffset);
	initInt8Array(this->staffProvinces, exeBytes, staffProvincesOffset);
	initInt8Array(this->climates, exeBytes, climatesOffset);
	initInt8Array(this->weatherTable, exeBytes, weatherTableOffset);
	init2DInt8Array(this->climateSpeedTables, exeBytes, climateSpeedTablesOffset);
	init2DInt8Array(this->weatherSpeedTables, exeBytes, weatherSpeedTablesOffset);
	initStringArrayNullTerminated(this->rulerTitles, exeBytes, rulerTitlesOffset);
	initStringArrayNullTerminated(this->distantMountainFilenames, exeBytes, distantMountainFilenamesOffset);
	initStringArrayNullTerminated(this->animDistantMountainFilenames, exeBytes, animDistantMountainFilenamesOffset);
	this->cloudFilename = GetExeStringNullTerminated(exeBytes, cloudFilenameOffset);
	this->sunFilename = GetExeStringNullTerminated(exeBytes, sunFilenameOffset);
	initStringArrayNullTerminated(this->moonFilenames, exeBytes, moonFilenamesOffset);
	this->starFilename = GetExeStringNullTerminated(exeBytes, starFilenameOffset);

	return true;
}

bool ExeDataLogbook::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Logbook";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int isEmptyOffset = GetExeAddress(*section, "IsEmpty");

	this->isEmpty = GetExeStringNullTerminated(exeBytes, isEmptyOffset);

	return true;
}

bool ExeDataMath::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Math";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int cosineTableOffset = GetExeAddress(*section, "CosineTable");

	initInt16Array(this->cosineTable, exeBytes, cosineTableOffset);

	return true;
}

bool ExeDataMeta::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Meta";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	this->dataSegmentOffset = GetExeAddress(*section, "DataSegmentOffset");

	return true;
}

bool ExeDataQuests::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Quests";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int mainQuestItemNamesOffset = GetExeAddress(*section, "MainQuestItemNames");
	const int staffPiecesOffset = GetExeAddress(*section, "StaffPieces");

	initStringArrayNullTerminated(this->mainQuestItemNames, exeBytes, mainQuestItemNamesOffset);
	this->staffPieces = GetExeStringNullTerminated(exeBytes, staffPiecesOffset);

	return true;
}

bool ExeDataRaces::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Races";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int singularNamesOffset = GetExeAddress(*section, "SingularNames");
	const int pluralNamesOffset = GetExeAddress(*section, "PluralNames");

	initStringArrayNullTerminated(this->singularNames, exeBytes, singularNamesOffset);
	initStringArrayNullTerminated(this->pluralNames, exeBytes, pluralNamesOffset);

	return true;
}

bool ExeDataRaisedPlatforms::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "RaisedPlatforms";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int boxArraysOffset = GetExeAddress(*section, "BoxArrays");
	const int boxArraysCopyOffset = GetExeAddress(*section, "BoxArraysCopy");
	const int box3aOffset = GetExeAddress(*section, "Box3A");
	const int box3bOffset = GetExeAddress(*section, "Box3B");
	const int box4Offset = GetExeAddress(*section, "Box4");

	initInt16Array(this->boxArrays, exeBytes, boxArraysOffset);
	initInt16Array(this->boxArraysCopy, exeBytes, boxArraysCopyOffset);
	initInt16Array(this->box3a, exeBytes, box3aOffset);
	initInt16Array(this->box3b, exeBytes, box3bOffset);
	initInt16Array(this->box4, exeBytes, box4Offset);

	this->heightsInterior.init(this->boxArrays, 8);
	this->heightsCity.init(this->boxArrays + 8, 8);
	this->heightsWild.init(this->boxArrays + 16, 8);
	this->thicknessesInterior.init(this->boxArrays + 24, 16);
	this->thicknessesCity.init(this->boxArrays + 40, 16); // Box2B is for city and wilderness.
	this->thicknessesWild = this->thicknessesCity;
	this->texMappingInterior.init(this->box3a);
	this->texMappingCity.init(this->box3b);
	this->texMappingWild.init(this->box4); // Treat Box4 as a Box3C.

	return true;
}

int ExeDataRaisedPlatforms::getTextureMappingValueA(MapType mapType, int heightIndex) const
{
	constexpr int maxTextureHeight = 64;

	switch (mapType)
	{
	case MapType::Interior:
		return this->texMappingInterior[heightIndex] % maxTextureHeight;
	case MapType::City:
		return this->texMappingCity[heightIndex] % maxTextureHeight;
	case MapType::Wilderness:
		return this->texMappingWild[heightIndex] % maxTextureHeight;
	default:
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(mapType)));
	}
}

int ExeDataRaisedPlatforms::getTextureMappingValueB(int thicknessIndex, int textureMappingValueA) const
{
	constexpr int maxTextureHeight = 64;
	DebugAssertIndex(this->box4, thicknessIndex);
	return maxTextureHeight - this->box4[thicknessIndex] - textureMappingValueA;
}

bool ExeDataServices::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Services";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int tavernRoomHealModifiersOffset = GetExeAddress(*section, "TavernRoomHealModifiers");

	initInt8Array(this->tavernRoomHealModifiers, exeBytes, tavernRoomHealModifiersOffset);

	return true;
}

bool ExeDataStatus::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Status";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int popUpOffset = GetExeAddress(*section, "PopUp");
	const int dateOffset = GetExeAddress(*section, "Date");
	const int fortifyOffset = GetExeAddress(*section, "Fortify");
	const int diseaseOffset = GetExeAddress(*section, "Disease");
	const int effectOffset = GetExeAddress(*section, "Effect");
	const int effectsListOffset = GetExeAddress(*section, "EffectsList");
	const int keyNamesOffset = GetExeAddress(*section, "KeyNames");
	const int keyPickedUpOffset = GetExeAddress(*section, "KeyPickedUp");
	const int doorUnlockedWithKeyOffset = GetExeAddress(*section, "DoorUnlockedWithKey");
	const int lockDifficultyMessagesOffset = GetExeAddress(*section, "LockDifficultyMessages");
	const int staminaExhaustedRecoverOffset = GetExeAddress(*section, "StaminaExhaustedRecover");
	const int staminaExhaustedDeathOffset = GetExeAddress(*section, "StaminaExhaustedDeath");
	const int staminaDrowningOffset = GetExeAddress(*section, "StaminaDrowning");
	const int enemyCorpseEmptyInventoryOffset = GetExeAddress(*section, "EnemyCorpseEmptyInventory");
	const int enemyCorpseGoldOffset = GetExeAddress(*section, "EnemyCorpseGold");
	const int citizenCorpseGoldOffset = GetExeAddress(*section, "CitizenCorpseGold");

	this->popUp = GetExeStringNullTerminated(exeBytes, popUpOffset);
	this->date = GetExeStringNullTerminated(exeBytes, dateOffset);
	this->fortify = GetExeStringNullTerminated(exeBytes, fortifyOffset);
	this->disease = GetExeStringNullTerminated(exeBytes, diseaseOffset);
	this->effect = GetExeStringNullTerminated(exeBytes, effectOffset);
	initStringArrayNullTerminated(this->effectsList, exeBytes, effectsListOffset);
	initStringArrayNullTerminated(this->keyNames, exeBytes, keyNamesOffset);
	this->keyPickedUp = GetExeStringNullTerminated(exeBytes, keyPickedUpOffset);
	this->doorUnlockedWithKey = GetExeStringNullTerminated(exeBytes, doorUnlockedWithKeyOffset);
	initStringArrayNullTerminated(this->lockDifficultyMessages, exeBytes, lockDifficultyMessagesOffset);
	this->staminaExhaustedRecover = GetExeStringNullTerminated(exeBytes, staminaExhaustedRecoverOffset);
	this->staminaExhaustedDeath = GetExeStringNullTerminated(exeBytes, staminaExhaustedDeathOffset);
	this->staminaDrowning = GetExeStringNullTerminated(exeBytes, staminaDrowningOffset);
	this->enemyCorpseEmptyInventory = GetExeStringNullTerminated(exeBytes, enemyCorpseEmptyInventoryOffset);
	this->enemyCorpseGold = GetExeStringNullTerminated(exeBytes, enemyCorpseGoldOffset);
	this->citizenCorpseGold = GetExeStringNullTerminated(exeBytes, citizenCorpseGoldOffset);

	return true;
}

bool ExeDataTravel::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Travel";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int locationFormatTextsOffset = GetExeAddress(*section, "LocationFormatTexts");
	const int dayPredictionOffset = GetExeAddress(*section, "DayPrediction");
	const int distancePredictionOffset = GetExeAddress(*section, "DistancePrediction");
	const int arrivalDatePredictionOffset = GetExeAddress(*section, "ArrivalDatePrediction");
	const int alreadyAtDestinationOffset = GetExeAddress(*section, "AlreadyAtDestination");
	const int noDestinationOffset = GetExeAddress(*section, "NoDestination");
	const int arrivalPopUpLocationOffset = GetExeAddress(*section, "ArrivalPopUpLocation");
	const int arrivalPopUpDateOffset = GetExeAddress(*section, "ArrivalPopUpDate");
	const int arrivalPopUpDaysOffset = GetExeAddress(*section, "ArrivalPopUpDays");
	const int arrivalCenterProvinceLocationOffset = GetExeAddress(*section, "ArrivalCenterProvinceLocation");
	const int searchTitleTextOffset = GetExeAddress(*section, "SearchTitleText");
	const int staffDungeonSplashesOffset = GetExeAddress(*section, "StaffDungeonSplashes");
	const int staffDungeonSplashIndicesOffset = GetExeAddress(*section, "StaffDungeonSplashIndices");

	initStringArrayNullTerminated(this->locationFormatTexts, exeBytes, locationFormatTextsOffset);
	initStringArrayNullTerminated(this->dayPrediction, exeBytes, dayPredictionOffset);
	this->distancePrediction = GetExeStringNullTerminated(exeBytes, distancePredictionOffset);
	this->arrivalDatePrediction = GetExeStringNullTerminated(exeBytes, arrivalDatePredictionOffset);
	this->alreadyAtDestination = GetExeStringNullTerminated(exeBytes, alreadyAtDestinationOffset);
	this->noDestination = GetExeStringNullTerminated(exeBytes, noDestinationOffset);
	this->arrivalPopUpLocation = GetExeStringNullTerminated(exeBytes, arrivalPopUpLocationOffset);
	this->arrivalPopUpDate = GetExeStringNullTerminated(exeBytes, arrivalPopUpDateOffset);
	this->arrivalPopUpDays = GetExeStringNullTerminated(exeBytes, arrivalPopUpDaysOffset);
	this->arrivalCenterProvinceLocation = GetExeStringNullTerminated(exeBytes, arrivalCenterProvinceLocationOffset);
	this->searchTitleText = GetExeStringNullTerminated(exeBytes, searchTitleTextOffset);
	initStringArrayNullTerminated(this->staffDungeonSplashes, exeBytes, staffDungeonSplashesOffset);
	initInt8Array(this->staffDungeonSplashIndices, exeBytes, staffDungeonSplashIndicesOffset);

	return true;
}

bool ExeDataUI::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "UI";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int chooseClassListOffset = GetExeAddress(*section, "ChooseClassList");
	const int buyingWeaponsOffset = GetExeAddress(*section, "BuyingWeapons");
	const int buyingArmorOffset = GetExeAddress(*section, "BuyingArmor");
	const int spellmakerOffset = GetExeAddress(*section, "Spellmaker");
	const int popUp5Offset = GetExeAddress(*section, "PopUp5");
	const int loadSaveOffset = GetExeAddress(*section, "LoadSave");
	const int charClassSelectionOffset = GetExeAddress(*section, "CharacterClassSelection");
	const int buyingMagicItemsOffset = GetExeAddress(*section, "BuyingMagicItems");
	const int travelCitySelectionOffset = GetExeAddress(*section, "TravelCitySelection");
	const int dialogueOffset = GetExeAddress(*section, "Dialogue");
	const int roomSelectionAndCuresOffset = GetExeAddress(*section, "RoomSelectionAndCures");
	const int generalLootAndSellingOffset = GetExeAddress(*section, "GeneralLootAndSelling");
	const int followerPortraitPositionsOffset = GetExeAddress(*section, "FollowerPortraitPositions");
	const int maleArmorClassPositionsOffset = GetExeAddress(*section, "MaleArmorClassPositions");
	const int femaleArmorClassPositionsOffset = GetExeAddress(*section, "FemaleArmorClassPositions");
	const int helmetPaletteIndicesOffset = GetExeAddress(*section, "HelmetPaletteIndices");
	const int race1HelmetPaletteValuesOffset = GetExeAddress(*section, "Race1HelmetPaletteValues");
	const int race3HelmetPaletteValuesOffset = GetExeAddress(*section, "Race3HelmetPaletteValues");
	const int race4HelmetPaletteValuesOffset = GetExeAddress(*section, "Race4HelmetPaletteValues");
	const int currentWorldPositionOffset = GetExeAddress(*section, "CurrentWorldPosition");
	const int inspectedEntityNameOffset = GetExeAddress(*section, "InspectedEntityName");

	this->chooseClassList.init(exeBytes, chooseClassListOffset);
	this->buyingWeapons.init(exeBytes, buyingWeaponsOffset);
	this->buyingArmor.init(exeBytes, buyingArmorOffset);
	this->spellmaker.init(exeBytes, spellmakerOffset);
	this->popUp5.init(exeBytes, popUp5Offset);
	this->loadSave.init(exeBytes, loadSaveOffset);
	this->charClassSelection.init(exeBytes, charClassSelectionOffset);
	this->buyingMagicItems.init(exeBytes, buyingMagicItemsOffset);
	this->travelCitySelection.init(exeBytes, travelCitySelectionOffset);
	this->dialogue.init(exeBytes, dialogueOffset);
	this->roomSelectionAndCures.init(exeBytes, roomSelectionAndCuresOffset);
	this->generalLootAndSelling.init(exeBytes, generalLootAndSellingOffset);
	initInt16Array(this->followerPortraitPositions, exeBytes, followerPortraitPositionsOffset);
	initInt16Array(this->maleArmorClassPositions, exeBytes, maleArmorClassPositionsOffset);
	initInt16Array(this->femaleArmorClassPositions, exeBytes, femaleArmorClassPositionsOffset);
	initInt8Array(this->helmetPaletteIndices, exeBytes, helmetPaletteIndicesOffset);
	initInt8Array(this->race1HelmetPaletteValues, exeBytes, race1HelmetPaletteValuesOffset);
	initInt8Array(this->race3HelmetPaletteValues, exeBytes, race3HelmetPaletteValuesOffset);
	initInt8Array(this->race4HelmetPaletteValues, exeBytes, race4HelmetPaletteValuesOffset);
	this->currentWorldPosition = GetExeStringNullTerminated(exeBytes, currentWorldPositionOffset);
	this->inspectedEntityName = GetExeStringNullTerminated(exeBytes, inspectedEntityNameOffset);

	return true;
}

bool ExeDataWeather::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Weather";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	const int fogTxtSampleHelperOffset = GetExeAddress(*section, "FogTxtSampleHelper");
	const int thunderstormFlashColorsOffset = GetExeAddress(*section, "ThunderstormFlashColors");

	initInt16Array(this->fogTxtSampleHelper, exeBytes, fogTxtSampleHelperOffset);
	initInt8Array(this->thunderstormFlashColors, exeBytes, thunderstormFlashColorsOffset);

	return true;
}

bool ExeDataWilderness::init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Wilderness";
	const KeyValueFileSection *section = keyValueFile.findSection(sectionName);
	if (section == nullptr)
	{
		DebugLogWarningFormat("Couldn't find \"%s\" section in .exe strings file.", sectionName.c_str());
		return false;
	}

	auto initWildBlockList = [](Buffer<uint8_t> &buffer, Span<const std::byte> exeBytes, int exeAddress)
	{
		// Each wilderness block list starts with the list size.
		const uint8_t listSize = static_cast<uint8_t>(exeBytes[exeAddress]);
		DebugAssert(exeBytes.isValidRange(exeAddress, listSize + 1));

		buffer.init(listSize);

		const uint8_t *listStart = reinterpret_cast<const uint8_t*>(exeBytes.begin()) + exeAddress + 1;
		const uint8_t *listEnd = listStart + listSize;
		std::copy(listStart, listEnd, buffer.begin());
	};

	const int normalBlocksOffset = GetExeAddress(*section, "NormalBlocks");
	const int villageBlocksOffset = GetExeAddress(*section, "VillageBlocks");
	const int dungeonBlocksOffset = GetExeAddress(*section, "DungeonBlocks");
	const int tavernBlocksOffset = GetExeAddress(*section, "TavernBlocks");
	const int templeBlocksOffset = GetExeAddress(*section, "TempleBlocks");

	initWildBlockList(this->normalBlocks, exeBytes, normalBlocksOffset);
	initWildBlockList(this->villageBlocks, exeBytes, villageBlocksOffset);
	initWildBlockList(this->dungeonBlocks, exeBytes, dungeonBlocksOffset);
	initWildBlockList(this->tavernBlocks, exeBytes, tavernBlocksOffset);
	initWildBlockList(this->templeBlocks, exeBytes, templeBlocksOffset);

	return true;
}

const std::string ExeData::FLOPPY_VERSION_EXE_FILENAME = "A.EXE";
const std::string ExeData::FLOPPY_VERSION_MAP_FILENAME = "data/text/aExeStrings.txt";
const std::string ExeData::CD_VERSION_EXE_FILENAME = "ACD.EXE";
const std::string ExeData::CD_VERSION_MAP_FILENAME = "data/text/acdExeStrings.txt";

ExeData::ExeData()
{
	this->isFloppyVersion = false;
}

bool ExeData::init(bool floppyVersion)
{
	const std::string &exeFilename = floppyVersion ? ExeData::FLOPPY_VERSION_EXE_FILENAME : ExeData::CD_VERSION_EXE_FILENAME;
	ExeUnpacker exe;
	if (!exe.init(exeFilename.c_str()))
	{
		DebugLogErrorFormat("Couldn't init .EXE unpacker for \"%s\".", exeFilename.c_str());
		return false;
	}

	const Span<const std::byte> exeBytes(reinterpret_cast<const std::byte*>(exe.getData().begin()), exe.getData().getCount());

	const std::string &mapFilename = floppyVersion ? ExeData::FLOPPY_VERSION_MAP_FILENAME : ExeData::CD_VERSION_MAP_FILENAME;
	KeyValueFile keyValueFile;
	if (!keyValueFile.init((Platform::getBasePath() + mapFilename).c_str()))
	{
		DebugLogErrorFormat("Couldn't init KeyValueFile for \"%s\".", exeFilename.c_str());
		return false;
	}

	bool success = this->calendar.init(exeBytes, keyValueFile);
	success &= this->charClasses.init(exeBytes, keyValueFile);
	success &= this->charCreation.init(exeBytes, keyValueFile);
	success &= this->cityGen.init(exeBytes, keyValueFile);
	success &= this->entities.init(exeBytes, keyValueFile);
	success &= this->equipment.init(exeBytes, keyValueFile);
	success &= this->items.init(exeBytes, keyValueFile);
	success &= this->light.init(exeBytes, keyValueFile);
	success &= this->locations.init(exeBytes, keyValueFile);
	success &= this->logbook.init(exeBytes, keyValueFile);
	success &= this->math.init(exeBytes, keyValueFile);
	success &= this->meta.init(exeBytes, keyValueFile);
	success &= this->quests.init(exeBytes, keyValueFile);
	success &= this->races.init(exeBytes, keyValueFile);
	success &= this->status.init(exeBytes, keyValueFile);
	success &= this->travel.init(exeBytes, keyValueFile);
	success &= this->ui.init(exeBytes, keyValueFile);
	success &= this->raisedPlatforms.init(exeBytes, keyValueFile);
	success &= this->weather.init(exeBytes, keyValueFile);
	success &= this->wild.init(exeBytes, keyValueFile);

	if (!success)
	{
		DebugLogError("Couldn't initialize one or more sections of ExeData.");
		return false;
	}

	this->isFloppyVersion = floppyVersion;

	return true;
}
