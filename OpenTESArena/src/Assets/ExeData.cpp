#include <algorithm>
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

	template<typename T, size_t U>
	void initInt8Array(T (&arr)[U], const char *data)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < std::size(arr); i++)
		{
			arr[i] = static_cast<T>(*(data + i));
		}
	}

	template<typename T, size_t U>
	void initInt8PairArray(std::pair<T, T> (&arr)[U], const char *data)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < std::size(arr); i++)
		{
			std::pair<T, T> &pair = arr[i];
			pair.first = static_cast<T>(*(data + (i * 2)));
			pair.second = static_cast<T>(*(data + ((i * 2) + 1)));
		}
	}

	template<typename T, size_t U>
	void initJaggedInt8Array(std::vector<T> (&arr)[U], T terminator, const char *data)
	{
		static_assert(sizeof(T) == 1);

		size_t offset = 0;
		for (std::vector<T> &vec : arr)
		{
			const char *innerData = data + offset;
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

	template<typename T, size_t U, size_t V>
	void init2DInt8Array(T (&arrs)[V][U], const char *data)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < std::size(arrs); i++)
		{
			initInt8Array(arrs[i], data + (i * U));
		}
	}

	template<typename T, size_t U>
	void initInt16Array(T (&arr)[U], const char *data)
	{
		static_assert(sizeof(T) == 2);

		for (size_t i = 0; i < std::size(arr); i++)
		{
			arr[i] = static_cast<T>(Bytes::getLE16(reinterpret_cast<const uint8_t*>(data + (i * 2))));
		}
	}

	template<typename T, size_t U>
	void initInt16PairArray(std::pair<T, T> (&arr)[U], const char *data)
	{
		static_assert(sizeof(T) == 2);
		const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);

		for (size_t i = 0; i < std::size(arr); i++)
		{
			std::pair<T, T> &pair = arr[i];
			pair.first = static_cast<T>(Bytes::getLE16(ptr + (i * 4)));
			pair.second = static_cast<T>(Bytes::getLE16(ptr + ((i * 4) + 2)));
		}
	}

	template<typename T, size_t U>
	void initInt32Array(T (&arr)[U], const char *data)
	{
		static_assert(sizeof(T) == 4);

		for (size_t i = 0; i < std::size(arr); i++)
		{
			arr[i] = static_cast<T>(Bytes::getLE32(reinterpret_cast<const uint8_t*>(data + (i * 4))));
		}
	}

	template<typename T, size_t U>
	void initIndexArray(int (&indexArr)[U], const T (&arr)[U])
	{
		// Construct an array of unique, sorted offsets based on the const array.
		// Remove zeroes because they do not count as offsets (they represent "null").
		T uniqueArr[U];
		const auto uniqueBegin = std::begin(uniqueArr);
		const auto uniqueEnd = [&arr, uniqueBegin]()
		{
			const auto iter = std::remove_copy(std::begin(arr), std::end(arr), uniqueBegin, 0);
			std::sort(uniqueBegin, iter);
			return std::unique(uniqueBegin, iter);
		}();

		// For each offset, if it is non-zero, its position in the uniques array is
		// the index to put into the index array.
		for (size_t i = 0; i < std::size(arr); i++)
		{
			const T offset = arr[i];
			const int index = [uniqueBegin, uniqueEnd, offset]()
			{
				// If the offset is "null", return -1 (indicates no restrictions).
				if (offset == 0)
				{
					return -1;
				}
				else
				{
					// Find the position of the offset in the unique offsets array.
					const auto offsetIter = std::find(uniqueBegin, uniqueEnd, offset);
					return static_cast<int>(std::distance(uniqueBegin, offsetIter));
				}
			}();

			indexArr[i] = index;
		}
	}

	template<size_t T>
	void initStringArrayNullTerminated(std::string (&arr)[T], const char *data)
	{
		size_t offset = 0;
		for (std::string &str : arr)
		{
			str = std::string(data + offset);
			offset += str.size() + 1;
		}
	}

	int GetExeAddress(const KeyValueFileSection &section, const std::string &key)
	{
		std::string_view valueStr;
		if (!section.tryGetString(key, valueStr))
		{
			DebugLogError("Couldn't get \"" + key + "\" (section \"" + section.getName() + "\").");
			return 0;
		}

		// Make sure the value only has an offset and isn't an offset + length pair.
		DebugAssertMsg(valueStr.find(PAIR_SEPARATOR) == std::string_view::npos, "\"" + key + "\" (section \"" + section.getName() + "\") should only have an offset.");

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
			DebugLogError("Couldn't get \"" + key + "\" (section \"" + section.getName() + "\").");
			return std::make_pair(0, 0);
		}

		// Make sure the value has a comma-separated offset + length pair.
		std::array<std::string_view, 2> tokens;
		if (!StringView::splitExpected<2>(valueStr, PAIR_SEPARATOR, tokens))
		{
			DebugCrash("Invalid offset + length pair \"" + key + "\" (section \"" + section.getName() + "\").");
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

	std::string GetExeStringNullTerminated(const char *data)
	{
		return std::string(data);
	}

	std::string GetExeStringFixedLength(const char *data, const std::pair<int, int> &pair)
	{
		return std::string(data + pair.first, pair.second);
	}
}

bool ExeDataCalendar::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Calendar";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int monthNamesOffset = GetExeAddress(*section, "MonthNames");
	const int timesOfDayOffset = GetExeAddress(*section, "TimesOfDay");
	const int weekdayNamesOffset = GetExeAddress(*section, "WeekdayNames");
	const int holidayNamesOffset = GetExeAddress(*section, "HolidayNames");
	const int holidayDatesOffset = GetExeAddress(*section, "HolidayDates");

	initStringArrayNullTerminated(this->monthNames, data + monthNamesOffset);
	initStringArrayNullTerminated(this->timesOfDay, data + timesOfDayOffset);
	initStringArrayNullTerminated(this->weekdayNames, data + weekdayNamesOffset);
	initStringArrayNullTerminated(this->holidayNames, data + holidayNamesOffset);
	initInt16Array(this->holidayDates, data + holidayDatesOffset);

	return true;
}

bool ExeDataCharacterClasses::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "CharacterClasses";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
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
	const int lockpickingDivisorsOffset = GetExeAddress(*section, "LockpickingDivisors");
	const int preferredAttributesOffset = GetExeAddress(*section, "PreferredAttributes");
	const int magicClassIntelligenceMultipliersOffset = GetExeAddress(*section, "MagicClassIntelligenceMultipliers");

	initInt8Array(this->allowedArmors, data + allowedArmorsOffset);
	initInt16Array(this->allowedShields, data + allowedShieldsOffset);

	const uint8_t shieldTerminator = 0xFF;
	initJaggedInt8Array(this->allowedShieldsLists, shieldTerminator, data + allowedShieldsListsOffset);

	initInt16Array(this->allowedWeapons, data + allowedWeaponsOffset);

	const uint8_t weaponTerminator = 0xFF;
	initJaggedInt8Array(this->allowedWeaponsLists, weaponTerminator, data + allowedWeaponsListsOffset);

	initIndexArray(this->allowedShieldsIndices, this->allowedShields);
	initIndexArray(this->allowedWeaponsIndices, this->allowedWeapons);
	initStringArrayNullTerminated(this->classNames, data + classNamesOffset);
	initInt8Array(this->classNumbersToIDs, data + classNumbersToIDsOffset);
	initInt8Array(this->healthDice, data + healthDiceOffset);
	initInt16Array(this->initialExperienceCaps, data + initialExpCapsOffset);
	initInt8Array(this->lockpickingDivisors, data + lockpickingDivisorsOffset);
	initStringArrayNullTerminated(this->preferredAttributes, data + preferredAttributesOffset);
	initInt8Array(this->magicClassIntelligenceMultipliers, data + magicClassIntelligenceMultipliersOffset);

	return true;
}

bool ExeDataCharacterCreation::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "CharacterCreation";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
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

	this->chooseClassCreation = GetExeStringFixedLength(data, chooseClassCreationPair);
	this->chooseClassCreationGenerate = GetExeStringFixedLength(data, chooseClassCreationGeneratePair);
	this->chooseClassCreationSelect = GetExeStringFixedLength(data, chooseClassCreationSelectPair);
	this->classQuestionsIntro = GetExeStringFixedLength(data, classQuestionsIntroPair);
	this->suggestedClass = GetExeStringFixedLength(data, suggestedClassPair);
	this->chooseClassList = GetExeStringFixedLength(data, chooseClassListPair);
	this->chooseName = GetExeStringFixedLength(data, chooseNamePair);
	this->chooseGender = GetExeStringFixedLength(data, chooseGenderPair);
	this->chooseGenderMale = GetExeStringFixedLength(data, chooseGenderMalePair);
	this->chooseGenderFemale = GetExeStringFixedLength(data, chooseGenderFemalePair);
	this->chooseRace = GetExeStringFixedLength(data, chooseRacePair);
	this->confirmRace = GetExeStringFixedLength(data, confirmRacePair);
	this->confirmedRace1 = GetExeStringFixedLength(data, confirmedRace1Pair);
	this->confirmedRace2 = GetExeStringFixedLength(data, confirmedRace2Pair);
	this->confirmedRace3 = GetExeStringFixedLength(data, confirmedRace3Pair);
	this->confirmedRace4 = GetExeStringFixedLength(data, confirmedRace4Pair);
	this->distributeClassPoints = GetExeStringFixedLength(data, distributeClassPointsPair);
	this->chooseAttributes = GetExeStringFixedLength(data, chooseAttributesPair);
	this->chooseAttributesSave = GetExeStringFixedLength(data, chooseAttributesSavePair);
	this->chooseAttributesReroll = GetExeStringFixedLength(data, chooseAttributesRerollPair);
	this->chooseAttributesBonusPointsRemaining = GetExeStringNullTerminated(data + chooseAttributesBonusPointsRemainingOffset);
	this->chooseAppearance = GetExeStringFixedLength(data, chooseAppearancePair);

	return true;
}

bool ExeDataCityGeneration::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "CityGeneration";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
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

	initInt8Array(this->coastalCityList, data + coastalCityListOffset);
	initStringArrayNullTerminated(this->templateFilenames, data + cityTemplateFilenamesOffset);
	initInt8PairArray(this->startingPositions, data + startingPositionsOffset);

	const uint8_t blockTerminator = 0;
	initJaggedInt8Array(this->reservedBlockLists, blockTerminator, data + reservedBlockListsOffset);

	initStringArrayNullTerminated(this->tavernPrefixes, data + tavernPrefixesOffset);
	initStringArrayNullTerminated(this->tavernMarineSuffixes, data + tavernMarineSuffixesOffset);
	initStringArrayNullTerminated(this->tavernSuffixes, data + tavernSuffixesOffset);
	initStringArrayNullTerminated(this->templePrefixes, data + templePrefixesOffset);
	initStringArrayNullTerminated(this->temple1Suffixes, data + temple1SuffixesOffset);
	initStringArrayNullTerminated(this->temple2Suffixes, data + temple2SuffixesOffset);
	initStringArrayNullTerminated(this->temple3Suffixes, data + temple3SuffixesOffset);
	initStringArrayNullTerminated(this->equipmentPrefixes, data + equipmentPrefixesOffset);
	initStringArrayNullTerminated(this->equipmentSuffixes, data + equipmentSuffixesOffset);
	this->magesGuildMenuName = GetExeStringNullTerminated(data + magesGuildMenuNameOffset);

	return true;
}

bool ExeDataEntities::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Entities";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
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
	const int creatureAnimFilenamesOffset = GetExeAddress(*section, "CreatureAnimationFilenames");
	const int finalBossNameOffset = GetExeAddress(*section, "FinalBossName");
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

	initStringArrayNullTerminated(this->attributeNames, data + attributeNamesOffset);
	initStringArrayNullTerminated(this->creatureNames, data + creatureNamesOffset);
	initInt8Array(this->creatureLevels, data + creatureLevelsOffset);
	initInt16PairArray(this->creatureHitPoints, data + creatureHitPointsOffset);
	initInt32Array(this->creatureBaseExps, data + creatureBaseExpsOffset);
	initInt8Array(this->creatureExpMultipliers, data + creatureExpMultipliersOffset);
	initInt8Array(this->creatureSounds, data + creatureSoundsOffset);
	initStringArrayNullTerminated(this->creatureSoundNames, data + creatureSoundNamesOffset);
	initInt8PairArray(this->creatureDamages, data + creatureDamagesOffset);
	initInt16Array(this->creatureMagicEffects, data + creatureMagicEffectsOffset);
	initInt16Array(this->creatureScales, data + creatureScalesOffset);
	initInt8Array(this->creatureYOffsets, data + creatureYOffsetsOffset);
	initInt8Array(this->creatureHasNoCorpse, data + creatureHasNoCorpseOffset);
	initInt8Array(this->creatureBlood, data + creatureBloodOffset);
	initInt8Array(this->creatureDiseaseChances, data + creatureDiseaseChancesOffset);
	init2DInt8Array(this->creatureAttributes, data + creatureAttributesOffset);
	initStringArrayNullTerminated(this->creatureAnimationFilenames, data + creatureAnimFilenamesOffset);
	this->finalBossName = GetExeStringNullTerminated(data + finalBossNameOffset);
	init2DInt8Array(this->raceAttributes, data + raceAttributesOffset);
	init2DInt8Array(this->guardAttributes, data + guardAttributesOffset);
	initStringArrayNullTerminated(this->maleCitizenAnimationFilenames, data + maleCitizenAnimFilenamesOffset);
	initStringArrayNullTerminated(this->femaleCitizenAnimationFilenames, data + femaleCitizenAnimFilenamesOffset);
	initStringArrayNullTerminated(this->humanFilenameTypes, data + humanFilenameTypesOffset);
	initStringArrayNullTerminated(this->humanFilenameTemplates, data + humanFilenameTemplatesOffset);
	initStringArrayNullTerminated(this->cfaHumansWithWeaponAnimations, data + cfaHumansWithWeaponAnimsOffset);
	initStringArrayNullTerminated(this->cfaWeaponAnimations, data + cfaWeaponAnimationsOffset);
	initStringArrayNullTerminated(this->effectAnimations, data + effectAnimsOffset);
	initInt8Array(this->citizenColorBase, data + citizenColorBaseOffset);
	initInt8Array(this->citizenSkinColors, data + citizenSkinColorsOffset);

	return true;
}

bool ExeDataEquipment::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Equipment";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int enchantmentChancesOffset = GetExeAddress(*section, "EnchantmentChances");
	const int materialNamesOffset = GetExeAddress(*section, "MaterialNames");
	const int materialBonusesOffset = GetExeAddress(*section, "MaterialBonuses");
	const int materialChancesOffset = GetExeAddress(*section, "MaterialChances");
	const int materialPriceMultipliersOffset = GetExeAddress(*section, "MaterialPriceMultipliers");
	const int itemConditionNamesOffset = GetExeAddress(*section, "ItemConditionNames");
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

	initInt8Array(this->enchantmentChances, data + enchantmentChancesOffset);
	initStringArrayNullTerminated(this->materialNames, data + materialNamesOffset);
	initInt8Array(this->materialBonuses, data + materialBonusesOffset);
	initInt8Array(this->materialChances, data + materialChancesOffset);
	initInt16Array(this->materialPriceMultipliers, data + materialPriceMultipliersOffset);
	initStringArrayNullTerminated(this->itemConditionNames, data + itemConditionNamesOffset);
	initStringArrayNullTerminated(this->armorNames, data + armorNamesOffset);
	initStringArrayNullTerminated(this->plateArmorNames, data + plateArmorNamesOffset);
	initInt8Array(this->plateArmorQualities, data + plateArmorQualitiesOffset);
	initInt8Array(this->plateArmorBasePrices, data + plateArmorBasePricesOffset);
	initInt16Array(this->plateArmorWeights, data + plateArmorWeightsOffset);
	initStringArrayNullTerminated(this->chainArmorNames, data + chainArmorNamesOffset);
	initInt8Array(this->chainArmorQualities, data + chainArmorQualitiesOffset);
	initInt8Array(this->chainArmorBasePrices, data + chainArmorBasePricesOffset);
	initInt16Array(this->chainArmorWeights, data + chainArmorWeightsOffset);
	initStringArrayNullTerminated(this->leatherArmorNames, data + leatherArmorNamesOffset);
	initInt8Array(this->leatherArmorQualities, data + leatherArmorQualitiesOffset);
	initInt8Array(this->leatherArmorBasePrices, data + leatherArmorBasePricesOffset);
	initInt16Array(this->leatherArmorWeights, data + leatherArmorWeightsOffset);
	initInt8Array(this->shieldArmorClasses, data + shieldArmorClassesOffset);
	initStringArrayNullTerminated(this->armorEnchantmentNames, data + armorEnchantmentNamesOffset);
	initInt8Array(this->armorEnchantmentQualities, data + armorEnchantmentQualitiesOffset);
	initInt8Array(this->armorEnchantmentSpells, data + armorEnchantmentSpellsOffset);
	initInt16Array(this->armorEnchantmentBonusPrices, data + armorEnchantmentBonusPricesOffset);
	initStringArrayNullTerminated(this->weaponNames, data + weaponNamesOffset);
	initInt8Array(this->weaponQualities, data + weaponQualitiesOffset);
	initInt8Array(this->weaponBasePrices, data + weaponBasePricesOffset);
	initInt16Array(this->weaponWeights, data + weaponWeightsOffset);
	initInt8PairArray(this->weaponDamages, data + weaponDamagesOffset);
	initInt8Array(this->weaponHandednesses, data + weaponHandednessesOffset);
	initStringArrayNullTerminated(this->weaponEnchantmentNames, data + weaponEnchantmentNamesOffset);
	initInt8Array(this->weaponEnchantmentQualities, data + weaponEnchantmentQualitiesOffset);
	initInt8Array(this->weaponEnchantmentSpells, data + weaponEnchantmentSpellsOffset);
	initInt16Array(this->weaponEnchantmentBonusPrices, data + weaponEnchantmentBonusPricesOffset);
	initStringArrayNullTerminated(this->spellcastingItemNames, data + spellcastingItemNamesOffset);
	initInt8Array(this->spellcastingItemCumulativeChances, data + spellcastingItemCumulativeChancesOffset);
	initInt16Array(this->spellcastingItemBasePrices, data + spellcastingItemBasePricesOffset);
	initInt8PairArray(this->spellcastingItemChargeRanges, data + spellcastingItemChargeRangesOffset);
	initStringArrayNullTerminated(this->spellcastingItemAttackSpellNames, data + spellcastingItemAttackSpellNamesOffset);
	initInt8Array(this->spellcastingItemAttackSpellQualities, data + spellcastingItemAttackSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemAttackSpellSpells, data + spellcastingItemAttackSpellSpellsOffset);
	initInt16Array(this->spellcastingItemAttackSpellPricesPerCharge, data + spellcastingItemAttackSpellPricesPerChargeOffset);
	initStringArrayNullTerminated(this->spellcastingItemDefensiveSpellNames, data + spellcastingItemDefensiveSpellNamesOffset);
	initInt8Array(this->spellcastingItemDefensiveSpellQualities, data + spellcastingItemDefensiveSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemDefensiveSpellSpells, data + spellcastingItemDefensiveSpellSpellsOffset);
	initInt16Array(this->spellcastingItemDefensiveSpellPricesPerCharge, data + spellcastingItemDefensiveSpellPricesPerChargeOffset);
	initStringArrayNullTerminated(this->spellcastingItemMiscSpellNames, data + spellcastingItemMiscSpellNamesOffset);
	initInt8Array(this->spellcastingItemMiscSpellQualities, data + spellcastingItemMiscSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemMiscSpellSpells, data + spellcastingItemMiscSpellSpellsOffset);
	initInt16Array(this->spellcastingItemMiscSpellPricesPerCharge, data + spellcastingItemMiscSpellPricesPerChargeOffset);
	initStringArrayNullTerminated(this->enhancementItemNames, data + enhancementItemNamesOffset);
	initInt8Array(this->enhancementItemCumulativeChances, data + enhancementItemCumulativeChancesOffset);
	initInt16Array(this->enhancementItemBasePrices, data + enhancementItemBasePricesOffset);
	initStringArrayNullTerminated(this->potionNames, data + potionNamesOffset);
	this->unidentifiedPotionName = GetExeStringNullTerminated(data + unidentifiedPotionNameOffset);
	initStringArrayNullTerminated(this->bodyPartNames, data + bodyPartNamesOffset);
	initStringArrayNullTerminated(this->weaponAnimationFilenames, data + weaponAnimFilenamesOffset);

	return true;
}

bool ExeDataItems::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Items";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int goldPieceOffset = GetExeAddress(*section, "GoldPiece");
	const int bagOfGoldPiecesOffset = GetExeAddress(*section, "BagOfGoldPieces");

	this->goldPiece = GetExeStringNullTerminated(data + goldPieceOffset);
	this->bagOfGoldPieces = GetExeStringNullTerminated(data + bagOfGoldPiecesOffset);

	return true;
}

bool ExeDataLight::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Light";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int windowTwilightColorsOffset = GetExeAddress(*section, "WindowTwilightColors");
	const int waterTwilightLightLevelsOffset = GetExeAddress(*section, "WaterTwilightLightLevels");

	initInt8Array(this->windowTwilightColors, data + windowTwilightColorsOffset);
	initInt16Array(this->waterTwilightLightLevels, data + waterTwilightLightLevelsOffset);

	return true;
}

bool ExeDataLocations::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Locations";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
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
		this->provinceNames[i] = GetExeStringNullTerminated(data + provinceNamesOffset + (i * 98));
	}

	initStringArrayNullTerminated(this->charCreationProvinceNames, data + charCreationProvinceNamesOffset);
	initStringArrayNullTerminated(this->provinceImgFilenames, data + provinceImgFilenamesOffset);
	initStringArrayNullTerminated(this->locationTypes, data + locationTypesOffset);
	initStringArrayNullTerminated(this->menuMifPrefixes, data + menuMifPrefixesOffset);
	this->centerProvinceCityMifName = GetExeStringNullTerminated(data + centerProvinceCityMifNameOffset);
	this->startDungeonName = GetExeStringNullTerminated(data + startDungeonNameOffset);
	this->startDungeonMifName = GetExeStringNullTerminated(data + startDungeonMifNameOffset);
	this->finalDungeonMifName = GetExeStringNullTerminated(data + finalDungeonMifNameOffset);
	initInt8Array(this->staffProvinces, data + staffProvincesOffset);
	initInt8Array(this->climates, data + climatesOffset);
	initInt8Array(this->weatherTable, data + weatherTableOffset);
	init2DInt8Array(this->climateSpeedTables, data + climateSpeedTablesOffset);
	init2DInt8Array(this->weatherSpeedTables, data + weatherSpeedTablesOffset);
	initStringArrayNullTerminated(this->rulerTitles, data + rulerTitlesOffset);
	initStringArrayNullTerminated(this->distantMountainFilenames, data + distantMountainFilenamesOffset);
	initStringArrayNullTerminated(this->animDistantMountainFilenames, data + animDistantMountainFilenamesOffset);
	this->cloudFilename = GetExeStringNullTerminated(data + cloudFilenameOffset);
	this->sunFilename = GetExeStringNullTerminated(data + sunFilenameOffset);
	initStringArrayNullTerminated(this->moonFilenames, data + moonFilenamesOffset);
	this->starFilename = GetExeStringNullTerminated(data + starFilenameOffset);

	return true;
}

bool ExeDataLogbook::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Logbook";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int isEmptyOffset = GetExeAddress(*section, "IsEmpty");

	this->isEmpty = GetExeStringNullTerminated(data + isEmptyOffset);

	return true;
}

bool ExeDataMeta::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Meta";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	this->dataSegmentOffset = GetExeAddress(*section, "DataSegmentOffset");

	return true;
}

bool ExeDataQuests::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Quests";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int mainQuestItemNamesOffset = GetExeAddress(*section, "MainQuestItemNames");
	const int staffPiecesOffset = GetExeAddress(*section, "StaffPieces");

	initStringArrayNullTerminated(this->mainQuestItemNames, data + mainQuestItemNamesOffset);
	this->staffPieces = GetExeStringNullTerminated(data + staffPiecesOffset);

	return true;
}

bool ExeDataRaces::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Races";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int singularNamesOffset = GetExeAddress(*section, "SingularNames");
	const int pluralNamesOffset = GetExeAddress(*section, "PluralNames");

	initStringArrayNullTerminated(this->singularNames, data + singularNamesOffset);
	initStringArrayNullTerminated(this->pluralNames, data + pluralNamesOffset);

	return true;
}

bool ExeDataRaisedPlatforms::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "RaisedPlatforms";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int boxArraysOffset = GetExeAddress(*section, "BoxArrays");
	const int boxArraysCopyOffset = GetExeAddress(*section, "BoxArraysCopy");
	const int box3aOffset = GetExeAddress(*section, "Box3A");
	const int box3bOffset = GetExeAddress(*section, "Box3B");
	const int box4Offset = GetExeAddress(*section, "Box4");

	initInt16Array(this->boxArrays, data + boxArraysOffset);
	initInt16Array(this->boxArraysCopy, data + boxArraysCopyOffset);
	initInt16Array(this->box3a, data + box3aOffset);
	initInt16Array(this->box3b, data + box3bOffset);
	initInt16Array(this->box4, data + box4Offset);

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
		return this->texMappingInterior.get(heightIndex) % maxTextureHeight;
	case MapType::City:
		return this->texMappingCity.get(heightIndex) % maxTextureHeight;
	case MapType::Wilderness:
		return this->texMappingWild.get(heightIndex) % maxTextureHeight;
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

bool ExeDataStatus::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Status";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
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

	this->popUp = GetExeStringNullTerminated(data + popUpOffset);
	this->date = GetExeStringNullTerminated(data + dateOffset);
	this->fortify = GetExeStringNullTerminated(data + fortifyOffset);
	this->disease = GetExeStringNullTerminated(data + diseaseOffset);
	this->effect = GetExeStringNullTerminated(data + effectOffset);
	initStringArrayNullTerminated(this->effectsList, data + effectsListOffset);
	initStringArrayNullTerminated(this->keyNames, data + keyNamesOffset);
	this->keyPickedUp = GetExeStringNullTerminated(data + keyPickedUpOffset);
	this->doorUnlockedWithKey = GetExeStringNullTerminated(data + doorUnlockedWithKeyOffset);
	initStringArrayNullTerminated(this->lockDifficultyMessages, data + lockDifficultyMessagesOffset);
	this->staminaExhaustedRecover = GetExeStringNullTerminated(data + staminaExhaustedRecoverOffset);
	this->staminaExhaustedDeath = GetExeStringNullTerminated(data + staminaExhaustedDeathOffset);
	this->staminaDrowning = GetExeStringNullTerminated(data + staminaDrowningOffset);
	this->enemyCorpseEmptyInventory = GetExeStringNullTerminated(data + enemyCorpseEmptyInventoryOffset);
	this->enemyCorpseGold = GetExeStringNullTerminated(data + enemyCorpseGoldOffset);

	return true;
}

bool ExeDataTravel::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Travel";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
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

	initStringArrayNullTerminated(this->locationFormatTexts, data + locationFormatTextsOffset);
	initStringArrayNullTerminated(this->dayPrediction, data + dayPredictionOffset);
	this->distancePrediction = GetExeStringNullTerminated(data + distancePredictionOffset);
	this->arrivalDatePrediction = GetExeStringNullTerminated(data + arrivalDatePredictionOffset);
	this->alreadyAtDestination = GetExeStringNullTerminated(data + alreadyAtDestinationOffset);
	this->noDestination = GetExeStringNullTerminated(data + noDestinationOffset);
	this->arrivalPopUpLocation = GetExeStringNullTerminated(data + arrivalPopUpLocationOffset);
	this->arrivalPopUpDate = GetExeStringNullTerminated(data + arrivalPopUpDateOffset);
	this->arrivalPopUpDays = GetExeStringNullTerminated(data + arrivalPopUpDaysOffset);
	this->arrivalCenterProvinceLocation = GetExeStringNullTerminated(data + arrivalCenterProvinceLocationOffset);
	this->searchTitleText = GetExeStringNullTerminated(data + searchTitleTextOffset);
	initStringArrayNullTerminated(this->staffDungeonSplashes, data + staffDungeonSplashesOffset);
	initInt8Array(this->staffDungeonSplashIndices, data + staffDungeonSplashIndicesOffset);

	return true;
}

bool ExeDataUI::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "UI";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
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

	this->chooseClassList.init(data + chooseClassListOffset);
	this->buyingWeapons.init(data + buyingWeaponsOffset);
	this->buyingArmor.init(data + buyingArmorOffset);
	this->spellmaker.init(data + spellmakerOffset);
	this->popUp5.init(data + popUp5Offset);
	this->loadSave.init(data + loadSaveOffset);
	this->charClassSelection.init(data + charClassSelectionOffset);
	this->buyingMagicItems.init(data + buyingMagicItemsOffset);
	this->travelCitySelection.init(data + travelCitySelectionOffset);
	this->dialogue.init(data + dialogueOffset);
	this->roomSelectionAndCures.init(data + roomSelectionAndCuresOffset);
	this->generalLootAndSelling.init(data + generalLootAndSellingOffset);
	initInt16Array(this->followerPortraitPositions, data + followerPortraitPositionsOffset);
	initInt16Array(this->maleArmorClassPositions, data + maleArmorClassPositionsOffset);
	initInt16Array(this->femaleArmorClassPositions, data + femaleArmorClassPositionsOffset);
	initInt8Array(this->helmetPaletteIndices, data + helmetPaletteIndicesOffset);
	initInt8Array(this->race1HelmetPaletteValues, data + race1HelmetPaletteValuesOffset);
	initInt8Array(this->race3HelmetPaletteValues, data + race3HelmetPaletteValuesOffset);
	initInt8Array(this->race4HelmetPaletteValues, data + race4HelmetPaletteValuesOffset);
	this->currentWorldPosition = GetExeStringNullTerminated(data + currentWorldPositionOffset);
	this->inspectedEntityName = GetExeStringNullTerminated(data + inspectedEntityNameOffset);

	return true;
}

bool ExeDataWeather::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Weather";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int thunderstormFlashColorsOffset = GetExeAddress(*section, "ThunderstormFlashColors");
	
	initInt8Array(this->thunderstormFlashColors, data + thunderstormFlashColorsOffset);

	return true;
}

bool ExeDataWilderness::init(const char *data, const KeyValueFile &keyValueFile)
{
	const std::string sectionName = "Wilderness";
	const KeyValueFileSection *section = keyValueFile.getSectionByName(sectionName);
	if (section == nullptr)
	{
		DebugLogWarning("Couldn't find \"" + sectionName + "\" section in .exe strings file.");
		return false;
	}

	const int normalBlocksOffset = GetExeAddress(*section, "NormalBlocks");
	const int villageBlocksOffset = GetExeAddress(*section, "VillageBlocks");
	const int dungeonBlocksOffset = GetExeAddress(*section, "DungeonBlocks");
	const int tavernBlocksOffset = GetExeAddress(*section, "TavernBlocks");
	const int templeBlocksOffset = GetExeAddress(*section, "TempleBlocks");

	auto initWildBlockList = [](std::vector<uint8_t> &vec, const char *data)
	{
		// Each wilderness block list starts with the list size.
		vec.resize(static_cast<uint8_t>(*data));

		const uint8_t *listStart = reinterpret_cast<const uint8_t*>(data + 1);
		const uint8_t *listEnd = listStart + vec.size();
		std::copy(listStart, listEnd, vec.data());
	};

	initWildBlockList(this->normalBlocks, data + normalBlocksOffset);
	initWildBlockList(this->villageBlocks, data + villageBlocksOffset);
	initWildBlockList(this->dungeonBlocks, data + dungeonBlocksOffset);
	initWildBlockList(this->tavernBlocks, data + tavernBlocksOffset);
	initWildBlockList(this->templeBlocks, data + templeBlocksOffset);

	return true;
}

const std::string ExeData::FLOPPY_VERSION_EXE_FILENAME = "A.EXE";
const std::string ExeData::FLOPPY_VERSION_MAP_FILENAME = "data/text/aExeStrings.txt";
const std::string ExeData::CD_VERSION_EXE_FILENAME = "ACD.EXE";
const std::string ExeData::CD_VERSION_MAP_FILENAME = "data/text/acdExeStrings.txt";

bool ExeData::init(bool floppyVersion)
{
	const std::string &exeFilename = floppyVersion ? ExeData::FLOPPY_VERSION_EXE_FILENAME : ExeData::CD_VERSION_EXE_FILENAME;
	ExeUnpacker exe;
	if (!exe.init(exeFilename.c_str()))
	{
		DebugLogError("Couldn't init .EXE unpacker for \"" + exeFilename + "\".");
		return false;
	}

	const char *dataPtr = reinterpret_cast<const char*>(exe.getData().begin());

	const std::string &mapFilename = floppyVersion ? ExeData::FLOPPY_VERSION_MAP_FILENAME : ExeData::CD_VERSION_MAP_FILENAME;
	KeyValueFile keyValueFile;
	if (!keyValueFile.init((Platform::getBasePath() + mapFilename).c_str()))
	{
		DebugLogError("Couldn't init KeyValueFile for \"" + exeFilename + "\".");
		return false;
	}

	bool success = this->calendar.init(dataPtr, keyValueFile);
	success &= this->charClasses.init(dataPtr, keyValueFile);
	success &= this->charCreation.init(dataPtr, keyValueFile);
	success &= this->cityGen.init(dataPtr, keyValueFile);
	success &= this->entities.init(dataPtr, keyValueFile);
	success &= this->equipment.init(dataPtr, keyValueFile);
	success &= this->items.init(dataPtr, keyValueFile);
	success &= this->light.init(dataPtr, keyValueFile);
	success &= this->locations.init(dataPtr, keyValueFile);
	success &= this->logbook.init(dataPtr, keyValueFile);
	success &= this->meta.init(dataPtr, keyValueFile);
	success &= this->quests.init(dataPtr, keyValueFile);
	success &= this->races.init(dataPtr, keyValueFile);
	success &= this->status.init(dataPtr, keyValueFile);
	success &= this->travel.init(dataPtr, keyValueFile);
	success &= this->ui.init(dataPtr, keyValueFile);
	success &= this->raisedPlatforms.init(dataPtr, keyValueFile);
	success &= this->weather.init(dataPtr, keyValueFile);
	success &= this->wild.init(dataPtr, keyValueFile);

	if (!success)
	{
		DebugLogError("Couldn't initialize one or more sections of ExeData.");
		return false;
	}

	this->isFloppyVersion = floppyVersion;

	return true;
}
