#include <algorithm>
#include <sstream>

#include "ExeData.h"
#include "ExeUnpacker.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace
{
	// Convenience method for initializing an array of 8-bit integers.
	template <typename T, size_t U>
	void initInt8Array(std::array<T, U> &arr, const char *data)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < arr.size(); i++)
		{
			arr[i] = static_cast<T>(*(data + i));
		}
	}

	// Convenience method for initializing an array of 8-bit integer pairs.
	template <typename T, size_t U>
	void initInt8PairArray(std::array<std::pair<T, T>, U> &arr, const char *data)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < arr.size(); i++)
		{
			std::pair<T, T> &pair = arr[i];
			pair.first = static_cast<T>(*(data + (i * 2)));
			pair.second = static_cast<T>(*(data + ((i * 2) + 1)));
		}
	}

	// Convenience method for initializing a jagged array of 8-bit integers.
	template <typename T, size_t U>
	void initJaggedInt8Array(std::array<std::vector<T>, U> &arr, T terminator, const char *data)
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
					vec.push_back(value);
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

	// Convenience method for initializing a 2D array of 8-bit integers.
	template <typename T, size_t U, size_t V>
	void init2DInt8Array(std::array<std::array<T, U>, V> &arrs, const char *data)
	{
		static_assert(sizeof(T) == 1);

		for (size_t i = 0; i < arrs.size(); i++)
		{
			initInt8Array(arrs[i], data + (i * U));
		}
	}

	// Convenience method for initializing an array of 16-bit integers.
	template <typename T, size_t U>
	void initInt16Array(std::array<T, U> &arr, const char *data)
	{
		static_assert(sizeof(T) == 2);

		for (size_t i = 0; i < arr.size(); i++)
		{
			arr[i] = static_cast<T>(
				Bytes::getLE16(reinterpret_cast<const uint8_t*>(data + (i * 2))));
		}
	}

	// Convenience method for initializing an array of 16-bit integer pairs.
	template <typename T, size_t U>
	void initInt16PairArray(std::array<std::pair<T, T>, U> &arr, const char *data)
	{
		static_assert(sizeof(T) == 2);
		const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);

		for (size_t i = 0; i < arr.size(); i++)
		{
			std::pair<T, T> &pair = arr[i];
			pair.first = static_cast<T>(Bytes::getLE16(ptr + (i * 4)));
			pair.second = static_cast<T>(Bytes::getLE16(ptr + ((i * 4) + 2)));
		}
	}

	// Convenience method for initializing an array of 32-bit integers.
	template <typename T, size_t U>
	void initInt32Array(std::array<T, U> &arr, const char *data)
	{
		static_assert(sizeof(T) == 4);

		for (size_t i = 0; i < arr.size(); i++)
		{
			arr[i] = static_cast<T>(
				Bytes::getLE32(reinterpret_cast<const uint8_t*>(data + (i * 4))));
		}
	}

	// Convenience method for initializing an index array.
	template <typename T, size_t U>
	void initIndexArray(std::array<int, U> &indexArr, const std::array<T, U> &arr)
	{
		// Construct an array of unique, sorted offsets based on the const array.
		// Remove zeroes because they do not count as offsets (they represent "null").
		std::array<T, U> uniqueArr;
		const auto uniqueBegin = uniqueArr.begin();
		const auto uniqueEnd = [&arr, uniqueBegin]()
		{
			const auto iter = std::remove_copy(arr.begin(), arr.end(), uniqueBegin, 0);
			std::sort(uniqueBegin, iter);
			return std::unique(uniqueBegin, iter);
		}();

		// For each offset, if it is non-zero, its position in the uniques array is
		// the index to put into the index array.
		for (size_t i = 0; i < arr.size(); i++)
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

	// Convenience method for initializing an array of null-terminated strings.
	template <size_t T>
	void initStringArray(std::array<std::string, T> &arr, const char *data)
	{
		size_t offset = 0;
		for (std::string &str : arr)
		{
			str = std::string(data + offset);
			offset += str.size() + 1;
		}
	}
}

void ExeData::Calendar::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Calendar");
	const int monthNamesOffset = ExeData::get(section, "MonthNames");
	const int timesOfDayOffset = ExeData::get(section, "TimesOfDay");
	const int weekdayNamesOffset = ExeData::get(section, "WeekdayNames");
	const int holidayNamesOffset = ExeData::get(section, "HolidayNames");
	const int holidayDatesOffset = ExeData::get(section, "HolidayDates");

	initStringArray(this->monthNames, data + monthNamesOffset);
	initStringArray(this->timesOfDay, data + timesOfDayOffset);
	initStringArray(this->weekdayNames, data + weekdayNamesOffset);
	initStringArray(this->holidayNames, data + holidayNamesOffset);
	initInt16Array(this->holidayDates, data + holidayDatesOffset);
}

void ExeData::CharacterClasses::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("CharacterClasses");
	const int allowedArmorsOffset = ExeData::get(section, "AllowedArmors");
	const int allowedShieldsOffset = ExeData::get(section, "AllowedShields");
	const int allowedShieldsListsOffset = ExeData::get(section, "AllowedShieldsLists");
	const int allowedWeaponsOffset = ExeData::get(section, "AllowedWeapons");
	const int allowedWeaponsListsOffset = ExeData::get(section, "AllowedWeaponsLists");
	const int classNamesOffset = ExeData::get(section, "ClassNames");
	const int classNumbersToIDsOffset = ExeData::get(section, "ClassNumbersToIDs");
	const int healthDiceOffset = ExeData::get(section, "HealthDice");
	const int initialExpCapsOffset = ExeData::get(section, "InitialExperienceCaps");
	const int lockpickingDivisorsOffset = ExeData::get(section, "LockpickingDivisors");
	const int preferredAttributesOffset = ExeData::get(section, "PreferredAttributes");

	initInt8Array(this->allowedArmors, data + allowedArmorsOffset);
	initInt16Array(this->allowedShields, data + allowedShieldsOffset);

	const uint8_t shieldTerminator = 0xFF;
	initJaggedInt8Array(this->allowedShieldsLists, shieldTerminator,
		data + allowedShieldsListsOffset);

	initInt16Array(this->allowedWeapons, data + allowedWeaponsOffset);

	const uint8_t weaponTerminator = 0xFF;
	initJaggedInt8Array(this->allowedWeaponsLists, weaponTerminator,
		data + allowedWeaponsListsOffset);

	initIndexArray(this->allowedShieldsIndices, this->allowedShields);
	initIndexArray(this->allowedWeaponsIndices, this->allowedWeapons);
	initStringArray(this->classNames, data + classNamesOffset);
	initInt8Array(this->classNumbersToIDs, data + classNumbersToIDsOffset);
	initInt8Array(this->healthDice, data + healthDiceOffset);
	initInt16Array(this->initialExperienceCaps, data + initialExpCapsOffset);
	initInt8Array(this->lockpickingDivisors, data + lockpickingDivisorsOffset);
	initStringArray(this->preferredAttributes, data + preferredAttributesOffset);
}

void ExeData::CharacterCreation::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("CharacterCreation");
	const auto chooseClassCreationPair = ExeData::getPair(section, "ChooseClassCreation");
	const auto chooseClassCreationGeneratePair = ExeData::getPair(section, "ChooseClassCreationGenerate");
	const auto chooseClassCreationSelectPair = ExeData::getPair(section, "ChooseClassCreationSelect");
	const auto classQuestionsIntroPair = ExeData::getPair(section, "ClassQuestionsIntro");
	const auto suggestedClassPair = ExeData::getPair(section, "SuggestedClass");
	const auto chooseClassListPair = ExeData::getPair(section, "ChooseClassList");
	const auto chooseNamePair = ExeData::getPair(section, "ChooseName");
	const auto chooseGenderPair = ExeData::getPair(section, "ChooseGender");
	const auto chooseGenderMalePair = ExeData::getPair(section, "ChooseGenderMale");
	const auto chooseGenderFemalePair = ExeData::getPair(section, "ChooseGenderFemale");
	const auto chooseRacePair = ExeData::getPair(section, "ChooseRace");
	const auto confirmRacePair = ExeData::getPair(section, "ConfirmRace");
	const auto confirmedRace1Pair = ExeData::getPair(section, "ConfirmedRace1");
	const auto confirmedRace2Pair = ExeData::getPair(section, "ConfirmedRace2");
	const auto confirmedRace3Pair = ExeData::getPair(section, "ConfirmedRace3");
	const auto confirmedRace4Pair = ExeData::getPair(section, "ConfirmedRace4");
	const auto distributeClassPointsPair = ExeData::getPair(section, "DistributeClassPoints");
	const auto chooseAttributesPair = ExeData::getPair(section, "ChooseAttributes");
	const auto chooseAttributesSavePair = ExeData::getPair(section, "ChooseAttributesSave");
	const auto chooseAttributesRerollPair = ExeData::getPair(section, "ChooseAttributesReroll");
	const auto chooseAppearancePair = ExeData::getPair(section, "ChooseAppearance");

	this->chooseClassCreation = ExeData::readFixedString(data, chooseClassCreationPair);
	this->chooseClassCreationGenerate =
		ExeData::readFixedString(data, chooseClassCreationGeneratePair);
	this->chooseClassCreationSelect =
		ExeData::readFixedString(data, chooseClassCreationSelectPair);
	this->classQuestionsIntro = ExeData::readFixedString(data, classQuestionsIntroPair);
	this->suggestedClass = ExeData::readFixedString(data, suggestedClassPair);
	this->chooseClassList = ExeData::readFixedString(data, chooseClassListPair);
	this->chooseName = ExeData::readFixedString(data, chooseNamePair);
	this->chooseGender = ExeData::readFixedString(data, chooseGenderPair);
	this->chooseGenderMale = ExeData::readFixedString(data, chooseGenderMalePair);
	this->chooseGenderFemale = ExeData::readFixedString(data, chooseGenderFemalePair);
	this->chooseRace = ExeData::readFixedString(data, chooseRacePair);
	this->confirmRace = ExeData::readFixedString(data, confirmRacePair);
	this->confirmedRace1 = ExeData::readFixedString(data, confirmedRace1Pair);
	this->confirmedRace2 = ExeData::readFixedString(data, confirmedRace2Pair);
	this->confirmedRace3 = ExeData::readFixedString(data, confirmedRace3Pair);
	this->confirmedRace4 = ExeData::readFixedString(data, confirmedRace4Pair);
	this->distributeClassPoints = ExeData::readFixedString(data, distributeClassPointsPair);
	this->chooseAttributes = ExeData::readFixedString(data, chooseAttributesPair);
	this->chooseAttributesSave = ExeData::readFixedString(data, chooseAttributesSavePair);
	this->chooseAttributesReroll = ExeData::readFixedString(data, chooseAttributesRerollPair);
	this->chooseAppearance = ExeData::readFixedString(data, chooseAppearancePair);
}

void ExeData::CityGeneration::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("CityGeneration");
	const int coastalCityListOffset = ExeData::get(section, "CoastalCityList");
	const int cityTemplateFilenamesOffset = ExeData::get(section, "CityTemplateFilenames");
	const int startingPositionsOffset = ExeData::get(section, "StartingPositions");
	const int reservedBlockListsOffset = ExeData::get(section, "ReservedBlockLists");
	const int tavernPrefixesOffset = ExeData::get(section, "TavernPrefixes");
	const int tavernMarineSuffixesOffset = ExeData::get(section, "TavernMarineSuffixes");
	const int tavernSuffixesOffset = ExeData::get(section, "TavernSuffixes");
	const int templePrefixesOffset = ExeData::get(section, "TemplePrefixes");
	const int temple1SuffixesOffset = ExeData::get(section, "Temple1Suffixes");
	const int temple2SuffixesOffset = ExeData::get(section, "Temple2Suffixes");
	const int temple3SuffixesOffset = ExeData::get(section, "Temple3Suffixes");
	const int equipmentPrefixesOffset = ExeData::get(section, "EquipmentPrefixes");
	const int equipmentSuffixesOffset = ExeData::get(section, "EquipmentSuffixes");
	const int magesGuildMenuNameOffset = ExeData::get(section, "MagesGuildMenuName");

	initInt8Array(this->coastalCityList, data + coastalCityListOffset);
	initStringArray(this->templateFilenames, data + cityTemplateFilenamesOffset);
	initInt8PairArray(this->startingPositions, data + startingPositionsOffset);

	const uint8_t blockTerminator = 0;
	initJaggedInt8Array(this->reservedBlockLists, blockTerminator,
		data + reservedBlockListsOffset);

	initStringArray(this->tavernPrefixes, data + tavernPrefixesOffset);
	initStringArray(this->tavernMarineSuffixes, data + tavernMarineSuffixesOffset);
	initStringArray(this->tavernSuffixes, data + tavernSuffixesOffset);
	initStringArray(this->templePrefixes, data + templePrefixesOffset);
	initStringArray(this->temple1Suffixes, data + temple1SuffixesOffset);
	initStringArray(this->temple2Suffixes, data + temple2SuffixesOffset);
	initStringArray(this->temple3Suffixes, data + temple3SuffixesOffset);
	initStringArray(this->equipmentPrefixes, data + equipmentPrefixesOffset);
	initStringArray(this->equipmentSuffixes, data + equipmentSuffixesOffset);
	this->magesGuildMenuName = ExeData::readString(data + magesGuildMenuNameOffset);
}

void ExeData::Entities::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Entities");
	const int creatureNamesOffset = ExeData::get(section, "CreatureNames");
	const int creatureLevelsOffset = ExeData::get(section, "CreatureLevels");
	const int creatureHitPointsOffset = ExeData::get(section, "CreatureHitPoints");
	const int creatureBaseExpsOffset = ExeData::get(section, "CreatureBaseExperience");
	const int creatureExpMultipliersOffset = ExeData::get(section, "CreatureExperienceMultipliers");
	const int creatureSoundsOffset = ExeData::get(section, "CreatureSounds");
	const int creatureSoundNamesOffset = ExeData::get(section, "CreatureSoundNames");
	const int creatureDamagesOffset = ExeData::get(section, "CreatureDamages");
	const int creatureMagicEffectsOffset = ExeData::get(section, "CreatureMagicEffects");
	const int creatureScalesOffset = ExeData::get(section, "CreatureScales");
	const int creatureYOffsetsOffset = ExeData::get(section, "CreatureYOffsets");
	const int creatureHasNoCorpseOffset = ExeData::get(section, "CreatureHasNoCorpse");
	const int creatureBloodOffset = ExeData::get(section, "CreatureBlood");
	const int creatureDiseaseChancesOffset = ExeData::get(section, "CreatureDiseaseChances");
	const int creatureAttributesOffset = ExeData::get(section, "CreatureAttributes");
	const int creatureAnimFilenamesOffset = ExeData::get(section, "CreatureAnimationFilenames");
	const int finalBossNameOffset = ExeData::get(section, "FinalBossName");
	const int raceAttributesOffset = ExeData::get(section, "RaceAttributes");
	const int guardAttributesOffset = ExeData::get(section, "GuardAttributes");
	const int maleCitizenAnimFilenamesOffset = ExeData::get(section, "MaleCitizenAnimationFilenames");
	const int femaleCitizenAnimFilenamesOffset = ExeData::get(section, "FemaleCitizenAnimationFilenames");
	const int humanFilenameTypesOffset = ExeData::get(section, "HumanFilenameTypes");
	const int humanFilenameTemplatesOffset = ExeData::get(section, "HumanFilenameTemplates");
	const int cfaHumansWithWeaponAnimsOffset = ExeData::get(section, "CFAHumansWithWeaponAnimations");
	const int cfaWeaponAnimationsOffset = ExeData::get(section, "CFAWeaponAnimations");
	const int effectAnimsOffset = ExeData::get(section, "EffectAnimations");
	const int citizenColorBaseOffset = ExeData::get(section, "CitizenColorBase");
	const int citizenSkinColorsOffset = ExeData::get(section, "CitizenSkinColors");

	initStringArray(this->creatureNames, data + creatureNamesOffset);
	initInt8Array(this->creatureLevels, data + creatureLevelsOffset);
	initInt16PairArray(this->creatureHitPoints, data + creatureHitPointsOffset);
	initInt32Array(this->creatureBaseExps, data + creatureBaseExpsOffset);
	initInt8Array(this->creatureExpMultipliers, data + creatureExpMultipliersOffset);
	initInt8Array(this->creatureSounds, data + creatureSoundsOffset);
	initStringArray(this->creatureSoundNames, data + creatureSoundNamesOffset);
	initInt8PairArray(this->creatureDamages, data + creatureDamagesOffset);
	initInt16Array(this->creatureMagicEffects, data + creatureMagicEffectsOffset);
	initInt16Array(this->creatureScales, data + creatureScalesOffset);
	initInt8Array(this->creatureYOffsets, data + creatureYOffsetsOffset);
	initInt8Array(this->creatureHasNoCorpse, data + creatureHasNoCorpseOffset);
	initInt8Array(this->creatureBlood, data + creatureBloodOffset);
	initInt8Array(this->creatureDiseaseChances, data + creatureDiseaseChancesOffset);
	init2DInt8Array(this->creatureAttributes, data + creatureAttributesOffset);
	initStringArray(this->creatureAnimationFilenames, data + creatureAnimFilenamesOffset);
	this->finalBossName = ExeData::readString(data + finalBossNameOffset);
	init2DInt8Array(this->raceAttributes, data + raceAttributesOffset);
	init2DInt8Array(this->guardAttributes, data + guardAttributesOffset);
	initStringArray(this->maleCitizenAnimationFilenames, data + maleCitizenAnimFilenamesOffset);
	initStringArray(this->femaleCitizenAnimationFilenames, data + femaleCitizenAnimFilenamesOffset);
	initStringArray(this->humanFilenameTypes, data + humanFilenameTypesOffset);
	initStringArray(this->humanFilenameTemplates, data + humanFilenameTemplatesOffset);
	initStringArray(this->cfaHumansWithWeaponAnimations, data + cfaHumansWithWeaponAnimsOffset);
	initStringArray(this->cfaWeaponAnimations, data + cfaWeaponAnimationsOffset);
	initStringArray(this->effectAnimations, data + effectAnimsOffset);
	initInt8Array(this->citizenColorBase, data + citizenColorBaseOffset);
	initInt8Array(this->citizenSkinColors, data + citizenSkinColorsOffset);
}

void ExeData::Equipment::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Equipment");
	const int enchantmentChancesOffset = ExeData::get(section, "EnchantmentChances");
	const int materialNamesOffset = ExeData::get(section, "MaterialNames");
	const int materialBonusesOffset = ExeData::get(section, "MaterialBonuses");
	const int materialChancesOffset = ExeData::get(section, "MaterialChances");
	const int materialPriceMultipliersOffset = ExeData::get(section, "MaterialPriceMultipliers");
	const int armorNamesOffset = ExeData::get(section, "ArmorNames");
	const int plateArmorNamesOffset = ExeData::get(section, "PlateArmorNames");
	const int plateArmorQualitiesOffset = ExeData::get(section, "PlateArmorQualities");
	const int plateArmorBasePricesOffset = ExeData::get(section, "PlateArmorBasePrices");
	const int plateArmorWeightsOffset = ExeData::get(section, "PlateArmorWeights");
	const int chainArmorNamesOffset = ExeData::get(section, "ChainArmorNames");
	const int chainArmorQualitiesOffset = ExeData::get(section, "ChainArmorQualities");
	const int chainArmorBasePricesOffset = ExeData::get(section, "ChainArmorBasePrices");
	const int chainArmorWeightsOffset = ExeData::get(section, "ChainArmorWeights");
	const int leatherArmorNamesOffset = ExeData::get(section, "LeatherArmorNames");
	const int leatherArmorQualitiesOffset = ExeData::get(section, "LeatherArmorQualities");
	const int leatherArmorBasePricesOffset = ExeData::get(section, "LeatherArmorBasePrices");
	const int leatherArmorWeightsOffset = ExeData::get(section, "LeatherArmorWeights");
	const int shieldArmorClassesOffset = ExeData::get(section, "ShieldArmorClasses");
	const int armorEnchantmentNamesOffset = ExeData::get(section, "ArmorEnchantmentNames");
	const int armorEnchantmentQualitiesOffset = ExeData::get(section, "ArmorEnchantmentQualities");
	const int armorEnchantmentSpellsOffset = ExeData::get(section, "ArmorEnchantmentSpells");
	const int armorEnchantmentBonusPricesOffset = ExeData::get(section, "ArmorEnchantmentBonusPrices");
	const int weaponNamesOffset = ExeData::get(section, "WeaponNames");
	const int weaponQualitiesOffset = ExeData::get(section, "WeaponQualities");
	const int weaponBasePricesOffset = ExeData::get(section, "WeaponBasePrices");
	const int weaponWeightsOffset = ExeData::get(section, "WeaponWeights");
	const int weaponDamagesOffset = ExeData::get(section, "WeaponDamages");
	const int weaponHandednessesOffset = ExeData::get(section, "WeaponHandednesses");
	const int weaponEnchantmentNamesOffset = ExeData::get(section, "WeaponEnchantmentNames");
	const int weaponEnchantmentQualitiesOffset = ExeData::get(section, "WeaponEnchantmentQualities");
	const int weaponEnchantmentSpellsOffset = ExeData::get(section, "WeaponEnchantmentSpells");
	const int weaponEnchantmentBonusPricesOffset = ExeData::get(section, "WeaponEnchantmentBonusPrices");
	const int spellcastingItemNamesOffset = ExeData::get(section, "SpellcastingItemNames");
	const int spellcastingItemCumulativeChancesOffset = ExeData::get(section, "SpellcastingItemCumulativeChances");
	const int spellcastingItemBasePricesOffset = ExeData::get(section, "SpellcastingItemBasePrices");
	const int spellcastingItemChargeRangesOffset = ExeData::get(section, "SpellcastingItemChargeRanges");
	const int spellcastingItemAttackSpellNamesOffset = ExeData::get(section, "SpellcastingItemAttackSpellNames");
	const int spellcastingItemAttackSpellQualitiesOffset = ExeData::get(section, "SpellcastingItemAttackSpellQualities");
	const int spellcastingItemAttackSpellSpellsOffset = ExeData::get(section, "SpellcastingItemAttackSpellSpells");
	const int spellcastingItemAttackSpellPricesPerChargeOffset = ExeData::get(section, "SpellcastingItemAttackSpellPricesPerCharge");
	const int spellcastingItemDefensiveSpellNamesOffset = ExeData::get(section, "SpellcastingItemDefensiveSpellNames");
	const int spellcastingItemDefensiveSpellQualitiesOffset = ExeData::get(section, "SpellcastingItemDefensiveSpellQualities");
	const int spellcastingItemDefensiveSpellSpellsOffset = ExeData::get(section, "SpellcastingItemDefensiveSpellSpells");
	const int spellcastingItemDefensiveSpellPricesPerChargeOffset = ExeData::get(section, "SpellcastingItemDefensiveSpellPricesPerCharge");
	const int spellcastingItemMiscSpellNamesOffset = ExeData::get(section, "SpellcastingItemMiscSpellNames");
	const int spellcastingItemMiscSpellQualitiesOffset = ExeData::get(section, "SpellcastingItemMiscSpellQualities");
	const int spellcastingItemMiscSpellSpellsOffset = ExeData::get(section, "SpellcastingItemMiscSpellSpells");
	const int spellcastingItemMiscSpellPricesPerChargeOffset = ExeData::get(section, "SpellcastingItemMiscSpellPricesPerCharge");
	const int enhancementItemNamesOffset = ExeData::get(section, "EnhancementItemNames");
	const int enhancementItemCumulativeChancesOffset = ExeData::get(section, "EnhancementItemCumulativeChances");
	const int enhancementItemBasePricesOffset = ExeData::get(section, "EnhancementItemBasePrices");
	const int bodyPartNamesOffset = ExeData::get(section, "BodyPartNames");
	const int weaponAnimFilenamesOffset = ExeData::get(section, "WeaponAnimationFilenames");

	initInt8Array(this->enchantmentChances, data + enchantmentChancesOffset);
	initStringArray(this->materialNames, data + materialNamesOffset);
	initInt8Array(this->materialBonuses, data + materialBonusesOffset);
	initInt8Array(this->materialChances, data + materialChancesOffset);
	initInt16Array(this->materialPriceMultipliers, data + materialPriceMultipliersOffset);
	initStringArray(this->armorNames, data + armorNamesOffset);
	initStringArray(this->plateArmorNames, data + plateArmorNamesOffset);
	initInt8Array(this->plateArmorQualities, data + plateArmorQualitiesOffset);
	initInt8Array(this->plateArmorBasePrices, data + plateArmorBasePricesOffset);
	initInt16Array(this->plateArmorWeights, data + plateArmorWeightsOffset);
	initStringArray(this->chainArmorNames, data + chainArmorNamesOffset);
	initInt8Array(this->chainArmorQualities, data + chainArmorQualitiesOffset);
	initInt8Array(this->chainArmorBasePrices, data + chainArmorBasePricesOffset);
	initInt16Array(this->chainArmorWeights, data + chainArmorWeightsOffset);
	initStringArray(this->leatherArmorNames, data + leatherArmorNamesOffset);
	initInt8Array(this->leatherArmorQualities, data + leatherArmorQualitiesOffset);
	initInt8Array(this->leatherArmorBasePrices, data + leatherArmorBasePricesOffset);
	initInt16Array(this->leatherArmorWeights, data + leatherArmorWeightsOffset);
	initInt8Array(this->shieldArmorClasses, data + shieldArmorClassesOffset);
	initStringArray(this->armorEnchantmentNames, data + armorEnchantmentNamesOffset);
	initInt8Array(this->armorEnchantmentQualities, data + armorEnchantmentQualitiesOffset);
	initInt8Array(this->armorEnchantmentSpells, data + armorEnchantmentSpellsOffset);
	initInt8Array(this->armorEnchantmentBonusPrices, data + armorEnchantmentBonusPricesOffset);
	initStringArray(this->weaponNames, data + weaponNamesOffset);
	initInt8Array(this->weaponQualities, data + weaponQualitiesOffset);
	initInt8Array(this->weaponBasePrices, data + weaponBasePricesOffset);
	initInt16Array(this->weaponWeights, data + weaponWeightsOffset);
	initInt8PairArray(this->weaponDamages, data + weaponDamagesOffset);
	initInt8Array(this->weaponHandednesses, data + weaponHandednessesOffset);
	initStringArray(this->weaponEnchantmentNames, data + weaponEnchantmentNamesOffset);
	initInt8Array(this->weaponEnchantmentQualities, data + weaponEnchantmentQualitiesOffset);
	initInt8Array(this->weaponEnchantmentSpells, data + weaponEnchantmentSpellsOffset);
	initInt8Array(this->weaponEnchantmentBonusPrices, data + weaponEnchantmentBonusPricesOffset);
	initStringArray(this->spellcastingItemNames, data + spellcastingItemNamesOffset);
	initInt8Array(this->spellcastingItemCumulativeChances, data + spellcastingItemCumulativeChancesOffset);
	initInt8Array(this->spellcastingItemBasePrices, data + spellcastingItemBasePricesOffset);
	initInt8PairArray(this->spellcastingItemChargeRanges, data + spellcastingItemChargeRangesOffset);
	initStringArray(this->spellcastingItemAttackSpellNames, data + spellcastingItemAttackSpellNamesOffset);
	initInt8Array(this->spellcastingItemAttackSpellQualities, data + spellcastingItemAttackSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemAttackSpellSpells, data + spellcastingItemAttackSpellSpellsOffset);
	initInt8Array(this->spellcastingItemAttackSpellPricesPerCharge, data + spellcastingItemAttackSpellPricesPerChargeOffset);
	initStringArray(this->spellcastingItemDefensiveSpellNames, data + spellcastingItemDefensiveSpellNamesOffset);
	initInt8Array(this->spellcastingItemDefensiveSpellQualities, data + spellcastingItemDefensiveSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemDefensiveSpellSpells, data + spellcastingItemDefensiveSpellSpellsOffset);
	initInt8Array(this->spellcastingItemDefensiveSpellPricesPerCharge, data + spellcastingItemDefensiveSpellPricesPerChargeOffset);
	initStringArray(this->spellcastingItemMiscSpellNames, data + spellcastingItemMiscSpellNamesOffset);
	initInt8Array(this->spellcastingItemMiscSpellQualities, data + spellcastingItemMiscSpellQualitiesOffset);
	initInt8Array(this->spellcastingItemMiscSpellSpells, data + spellcastingItemMiscSpellSpellsOffset);
	initInt8Array(this->spellcastingItemMiscSpellPricesPerCharge, data + spellcastingItemMiscSpellPricesPerChargeOffset);
	initStringArray(this->enhancementItemNames, data + enhancementItemNamesOffset);
	initInt8Array(this->enhancementItemCumulativeChances, data + enhancementItemCumulativeChancesOffset);
	initInt8Array(this->enhancementItemBasePrices, data + enhancementItemBasePricesOffset);
	initStringArray(this->bodyPartNames, data + bodyPartNamesOffset);
	initStringArray(this->weaponAnimationFilenames, data + weaponAnimFilenamesOffset);
}

void ExeData::Locations::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Locations");
	const int provinceNamesOffset = ExeData::get(section, "ProvinceNames");
	const int charCreationProvinceNamesOffset = ExeData::get(section, "CharCreationProvinceNames");
	const int provinceImgFilenamesOffset = ExeData::get(section, "ProvinceImgFilenames");
	const int locationTypesOffset = ExeData::get(section, "LocationTypes");
	const int menuMifPrefixesOffset = ExeData::get(section, "MenuMifPrefixes");
	const int centerProvinceCityMifNameOffset = ExeData::get(section, "CenterProvinceCityMifName");
	const int startDungeonNameOffset = ExeData::get(section, "StartDungeonName");
	const int startDungeonMifNameOffset = ExeData::get(section, "StartDungeonMifName");
	const int finalDungeonMifNameOffset = ExeData::get(section, "FinalDungeonMifName");
	const int staffProvincesOffset = ExeData::get(section, "StaffProvinces");
	const int climatesOffset = ExeData::get(section, "Climates");
	const int weatherTableOffset = ExeData::get(section, "WeatherTable");
	const int climateSpeedTablesOffset = ExeData::get(section, "ClimateSpeedTables");
	const int weatherSpeedTablesOffset = ExeData::get(section, "WeatherSpeedTables");
	const int rulerTitlesOffset = ExeData::get(section, "RulerTitles");
	const int distantMountainFilenamesOffset = ExeData::get(section, "DistantMountainFilenames");
	const int animDistantMountainFilenamesOffset = ExeData::get(section, "AnimDistantMountainFilenames");
	const int cloudFilenameOffset = ExeData::get(section, "CloudFilename");
	const int sunFilenameOffset = ExeData::get(section, "SunFilename");
	const int moonFilenamesOffset = ExeData::get(section, "MoonFilenames");
	const int starFilenameOffset = ExeData::get(section, "StarFilename");

	// Each province name is null-terminated and 98 bytes apart.
	for (size_t i = 0; i < this->provinceNames.size(); i++)
	{
		this->provinceNames[i] = ExeData::readString(data + provinceNamesOffset + (i * 98));
	}

	initStringArray(this->charCreationProvinceNames, data + charCreationProvinceNamesOffset);
	initStringArray(this->provinceImgFilenames, data + provinceImgFilenamesOffset);
	initStringArray(this->locationTypes, data + locationTypesOffset);
	initStringArray(this->menuMifPrefixes, data + menuMifPrefixesOffset);
	this->centerProvinceCityMifName = ExeData::readString(data + centerProvinceCityMifNameOffset);
	this->startDungeonName = ExeData::readString(data + startDungeonNameOffset);
	this->startDungeonMifName = ExeData::readString(data + startDungeonMifNameOffset);
	this->finalDungeonMifName = ExeData::readString(data + finalDungeonMifNameOffset);
	initInt8Array(this->staffProvinces, data + staffProvincesOffset);
	initInt8Array(this->climates, data + climatesOffset);
	initInt8Array(this->weatherTable, data + weatherTableOffset);
	init2DInt8Array(this->climateSpeedTables, data + climateSpeedTablesOffset);
	init2DInt8Array(this->weatherSpeedTables, data + weatherSpeedTablesOffset);
	initStringArray(this->rulerTitles, data + rulerTitlesOffset);
	initStringArray(this->distantMountainFilenames, data + distantMountainFilenamesOffset);
	initStringArray(this->animDistantMountainFilenames, data + animDistantMountainFilenamesOffset);
	this->cloudFilename = ExeData::readString(data + cloudFilenameOffset);
	this->sunFilename = ExeData::readString(data + sunFilenameOffset);
	initStringArray(this->moonFilenames, data + moonFilenamesOffset);
	this->starFilename = ExeData::readString(data + starFilenameOffset);
}

void ExeData::Logbook::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Logbook");
	const int isEmptyOffset = ExeData::get(section, "IsEmpty");

	this->isEmpty = ExeData::readString(data + isEmptyOffset);
}

void ExeData::Meta::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Meta");
	this->dataSegmentOffset = ExeData::get(section, "DataSegmentOffset");
}

void ExeData::Quests::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Quests");
	const int mainQuestItemNamesOffset = ExeData::get(section, "MainQuestItemNames");
	const int staffPiecesOffset = ExeData::get(section, "StaffPieces");
	const int keyNamesOffset = ExeData::get(section, "KeyNames");
	const int keyPickedUpOffset = ExeData::get(section, "KeyPickedUp");
	const int doorUnlockedWithKeyOffset = ExeData::get(section, "DoorUnlockedWithKey");

	initStringArray(this->mainQuestItemNames, data + mainQuestItemNamesOffset);
	this->staffPieces = ExeData::readString(data + staffPiecesOffset);
	initStringArray(this->keyNames, data + keyNamesOffset);
	this->keyPickedUp = ExeData::readString(data + keyPickedUpOffset);
	this->doorUnlockedWithKey = ExeData::readString(data + doorUnlockedWithKeyOffset);
}

void ExeData::Races::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Races");
	const int singularNamesOffset = ExeData::get(section, "SingularNames");
	const int pluralNamesOffset = ExeData::get(section, "PluralNames");

	initStringArray(this->singularNames, data + singularNamesOffset);
	initStringArray(this->pluralNames, data + pluralNamesOffset);
}

void ExeData::Status::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Status");
	const int popUpOffset = ExeData::get(section, "PopUp");
	const int dateOffset = ExeData::get(section, "Date");
	const int fortifyOffset = ExeData::get(section, "Fortify");
	const int diseaseOffset = ExeData::get(section, "Disease");
	const int effectOffset = ExeData::get(section, "Effect");
	const int effectsListOffset = ExeData::get(section, "EffectsList");

	this->popUp = ExeData::readString(data + popUpOffset);
	this->date = ExeData::readString(data + dateOffset);
	this->fortify = ExeData::readString(data + fortifyOffset);
	this->disease = ExeData::readString(data + diseaseOffset);
	this->effect = ExeData::readString(data + effectOffset);
	initStringArray(this->effectsList, data + effectsListOffset);
}

void ExeData::Travel::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Travel");
	const int locationFormatTextsOffset = ExeData::get(section, "LocationFormatTexts");
	const int dayPredictionOffset = ExeData::get(section, "DayPrediction");
	const int distancePredictionOffset = ExeData::get(section, "DistancePrediction");
	const int arrivalDatePredictionOffset = ExeData::get(section, "ArrivalDatePrediction");
	const int alreadyAtDestinationOffset = ExeData::get(section, "AlreadyAtDestination");
	const int noDestinationOffset = ExeData::get(section, "NoDestination");
	const int arrivalPopUpLocationOffset = ExeData::get(section, "ArrivalPopUpLocation");
	const int arrivalPopUpDateOffset = ExeData::get(section, "ArrivalPopUpDate");
	const int arrivalPopUpDaysOffset = ExeData::get(section, "ArrivalPopUpDays");
	const int arrivalCenterProvinceLocationOffset = ExeData::get(section, "ArrivalCenterProvinceLocation");
	const int searchTitleTextOffset = ExeData::get(section, "SearchTitleText");
	const int staffDungeonSplashesOffset = ExeData::get(section, "StaffDungeonSplashes");
	const int staffDungeonSplashIndicesOffset = ExeData::get(section, "StaffDungeonSplashIndices");

	initStringArray(this->locationFormatTexts, data + locationFormatTextsOffset);
	initStringArray(this->dayPrediction, data + dayPredictionOffset);
	this->distancePrediction = ExeData::readString(data + distancePredictionOffset);
	this->arrivalDatePrediction = ExeData::readString(data + arrivalDatePredictionOffset);
	this->alreadyAtDestination = ExeData::readString(data + alreadyAtDestinationOffset);
	this->noDestination = ExeData::readString(data + noDestinationOffset);
	this->arrivalPopUpLocation = ExeData::readString(data + arrivalPopUpLocationOffset);
	this->arrivalPopUpDate = ExeData::readString(data + arrivalPopUpDateOffset);
	this->arrivalPopUpDays = ExeData::readString(data + arrivalPopUpDaysOffset);
	this->arrivalCenterProvinceLocation = ExeData::readString(data + arrivalCenterProvinceLocationOffset);
	this->searchTitleText = ExeData::readString(data + searchTitleTextOffset);
	initStringArray(this->staffDungeonSplashes, data + staffDungeonSplashesOffset);
	initInt8Array(this->staffDungeonSplashIndices, data + staffDungeonSplashIndicesOffset);
}

void ExeData::UI::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("UI");
	const int chooseClassListOffset = ExeData::get(section, "ChooseClassList");
	const int buyingWeaponsOffset = ExeData::get(section, "BuyingWeapons");
	const int buyingArmorOffset = ExeData::get(section, "BuyingArmor");
	const int spellmakerOffset = ExeData::get(section, "Spellmaker");
	const int popUp5Offset = ExeData::get(section, "PopUp5");
	const int loadSaveOffset = ExeData::get(section, "LoadSave");
	const int charClassSelectionOffset = ExeData::get(section, "CharacterClassSelection");
	const int buyingMagicItemsOffset = ExeData::get(section, "BuyingMagicItems");
	const int travelCitySelectionOffset = ExeData::get(section, "TravelCitySelection");
	const int dialogueOffset = ExeData::get(section, "Dialogue");
	const int roomSelectionAndCuresOffset = ExeData::get(section, "RoomSelectionAndCures");
	const int generalLootAndSellingOffset = ExeData::get(section, "GeneralLootAndSelling");
	const int followerPortraitPositionsOffset = ExeData::get(section, "FollowerPortraitPositions");
	const int maleArmorClassPositionsOffset = ExeData::get(section, "MaleArmorClassPositions");
	const int femaleArmorClassPositionsOffset = ExeData::get(section, "FemaleArmorClassPositions");
	const int helmetPaletteIndicesOffset = ExeData::get(section, "HelmetPaletteIndices");
	const int race1HelmetPaletteValuesOffset = ExeData::get(section, "Race1HelmetPaletteValues");
	const int race3HelmetPaletteValuesOffset = ExeData::get(section, "Race3HelmetPaletteValues");
	const int race4HelmetPaletteValuesOffset = ExeData::get(section, "Race4HelmetPaletteValues");
	const int currentWorldPositionOffset = ExeData::get(section, "CurrentWorldPosition");
	const int inspectedEntityNameOffset = ExeData::get(section, "InspectedEntityName");

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
	this->currentWorldPosition = ExeData::readString(data + currentWorldPositionOffset);
	this->inspectedEntityName = ExeData::readString(data + inspectedEntityNameOffset);
}

void ExeData::WallHeightTables::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("WallHeightTables");
	const int box1aOffset = ExeData::get(section, "Box1A");
	const int box1bOffset = ExeData::get(section, "Box1B");
	const int box1cOffset = ExeData::get(section, "Box1C");
	const int box2aOffset = ExeData::get(section, "Box2A");
	const int box2bOffset = ExeData::get(section, "Box2B");
	const int box3aOffset = ExeData::get(section, "Box3A");
	const int box3bOffset = ExeData::get(section, "Box3B");
	const int box4Offset = ExeData::get(section, "Box4");

	initInt16Array(this->box1a, data + box1aOffset);
	initInt16Array(this->box1b, data + box1bOffset);
	initInt16Array(this->box1c, data + box1cOffset);
	initInt16Array(this->box2a, data + box2aOffset);
	initInt16Array(this->box2b, data + box2bOffset);
	initInt16Array(this->box3a, data + box3aOffset);
	initInt16Array(this->box3b, data + box3bOffset);
	initInt16Array(this->box4, data + box4Offset);
}

void ExeData::Wilderness::init(const char *data, const KeyValueFile &keyValueFile)
{
	const KeyValueFile::Section &section = *keyValueFile.getSectionByName("Wilderness");
	const int normalBlocksOffset = ExeData::get(section, "NormalBlocks");
	const int villageBlocksOffset = ExeData::get(section, "VillageBlocks");
	const int dungeonBlocksOffset = ExeData::get(section, "DungeonBlocks");
	const int tavernBlocksOffset = ExeData::get(section, "TavernBlocks");
	const int templeBlocksOffset = ExeData::get(section, "TempleBlocks");

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
}

const std::string ExeData::FLOPPY_VERSION_EXE_FILENAME = "A.EXE";
const std::string ExeData::FLOPPY_VERSION_MAP_FILENAME = "data/text/aExeStrings.txt";
const std::string ExeData::CD_VERSION_EXE_FILENAME = "ACD.EXE";
const std::string ExeData::CD_VERSION_MAP_FILENAME = "data/text/acdExeStrings.txt";

int ExeData::get(const KeyValueFile::Section &section, const std::string &key)
{
	std::string_view valueStr;
	if (!section.tryGetString(key, valueStr))
	{
		DebugCrash("Couldn't get \"" + key + "\" (section \"" + section.getName() + "\").");
	}

	// Make sure the value only has an offset and isn't an offset + length pair.
	DebugAssertMsg(valueStr.find(ExeData::PAIR_SEPARATOR) == std::string_view::npos,
		"\"" + key + "\" (section \"" + section.getName() + "\") should only have an offset.");

	int offset;

	std::stringstream ss;
	ss << std::hex << valueStr;
	ss >> offset;
	return offset;
}

std::pair<int, int> ExeData::getPair(const KeyValueFile::Section &section, const std::string &key)
{
	std::string_view valueStr;
	if (!section.tryGetString(key, valueStr))
	{
		DebugCrash("Couldn't get \"" + key + "\" (section \"" + section.getName() + "\").");
	}

	// Make sure the value has a comma-separated offset + length pair.
	std::array<std::string_view, 2> tokens;
	if (!StringView::splitExpected(valueStr, ExeData::PAIR_SEPARATOR, tokens))
	{
		DebugCrash("Invalid offset + length pair \"" + key + "\" (section \"" + section.getName() + "\").");
	}

	const std::string_view &offsetStr = tokens[0];
	const std::string_view &lengthStr = tokens[1];
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

int8_t ExeData::readInt8(const char *data)
{
	return static_cast<int8_t>(*data);
}

uint8_t ExeData::readUint8(const char *data)
{
	return static_cast<uint8_t>(*data);
}

int16_t ExeData::readInt16(const char *data)
{
	return static_cast<int16_t>(
		Bytes::getLE16(reinterpret_cast<const uint8_t*>(data)));
}

uint16_t ExeData::readUint16(const char *data)
{
	return Bytes::getLE16(reinterpret_cast<const uint8_t*>(data));
}

std::string ExeData::readString(const char *data)
{
	return std::string(data);
}

std::string ExeData::readFixedString(const char *data, const std::pair<int, int> &pair)
{
	return std::string(data + pair.first, pair.second);
}

bool ExeData::isFloppyVersion() const
{
	return this->floppyVersion;
}

bool ExeData::init(bool floppyVersion)
{
	// Load executable.
	const std::string &exeFilename = floppyVersion ?
		ExeData::FLOPPY_VERSION_EXE_FILENAME : ExeData::CD_VERSION_EXE_FILENAME;
	ExeUnpacker exe;
	if (!exe.init(exeFilename.c_str()))
	{
		DebugLogError("Couldn't init .EXE unpacker for \"" + exeFilename + "\".");
		return false;
	}

	const char *dataPtr = reinterpret_cast<const char*>(exe.getData().data());

	// Load key-value map file.
	const std::string &mapFilename = floppyVersion ?
		ExeData::FLOPPY_VERSION_MAP_FILENAME : ExeData::CD_VERSION_MAP_FILENAME;
	KeyValueFile keyValueFile;
	if (!keyValueFile.init((Platform::getBasePath() + mapFilename).c_str()))
	{
		DebugLogError("Couldn't init KeyValueFile for \"" + exeFilename + "\".");
		return false;
	}

	// Initialize members with the executable mappings.
	this->calendar.init(dataPtr, keyValueFile);
	this->charClasses.init(dataPtr, keyValueFile);
	this->charCreation.init(dataPtr, keyValueFile);
	this->cityGen.init(dataPtr, keyValueFile);
	this->entities.init(dataPtr, keyValueFile);
	this->equipment.init(dataPtr, keyValueFile);
	this->locations.init(dataPtr, keyValueFile);
	this->logbook.init(dataPtr, keyValueFile);
	this->meta.init(dataPtr, keyValueFile);
	this->quests.init(dataPtr, keyValueFile);
	this->races.init(dataPtr, keyValueFile);
	this->status.init(dataPtr, keyValueFile);
	this->travel.init(dataPtr, keyValueFile);
	this->ui.init(dataPtr, keyValueFile);
	this->wallHeightTables.init(dataPtr, keyValueFile);
	this->wild.init(dataPtr, keyValueFile);

	this->floppyVersion = floppyVersion;

	return true;
}
