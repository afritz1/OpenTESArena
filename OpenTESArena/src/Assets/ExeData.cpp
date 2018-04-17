#include <algorithm>
#include <sstream>

#include "ExeData.h"
#include "ExeUnpacker.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/KeyValueMap.h"
#include "../Utilities/Platform.h"
#include "../Utilities/String.h"

namespace
{
	// Convenience method for initializing an array of 8-bit integers.
	template <typename T, size_t U>
	void initInt8Array(std::array<T, U> &arr, const char *data)
	{
		static_assert(sizeof(T) == 1, "sizeof(T) != 1.");

		for (size_t i = 0; i < arr.size(); i++)
		{
			arr.at(i) = static_cast<T>(*(data + i));
		}
	}

	// Convenience method for initializing an array of 8-bit integer pairs.
	template <typename T, size_t U>
	void initInt8PairArray(std::array<std::pair<T, T>, U> &arr, const char *data)
	{
		static_assert(sizeof(T) == 1, "sizeof(T) != 1.");

		for (size_t i = 0; i < arr.size(); i++)
		{
			std::pair<T, T> &pair = arr.at(i);
			pair.first = static_cast<T>(*(data + (i * 2)));
			pair.second = static_cast<T>(*(data + ((i * 2) + 1)));
		}
	}

	// Convenience method for initializing a jagged array of 8-bit integers.
	template <typename T, size_t U>
	void initJaggedInt8Array(std::array<std::vector<T>, U> &arr, T terminator, const char *data)
	{
		static_assert(sizeof(T) == 1, "sizeof(T) != 1.");

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
		static_assert(sizeof(T) == 1, "sizeof(T) != 1.");

		for (size_t i = 0; i < arrs.size(); i++)
		{
			initInt8Array(arrs.at(i), data + (i * U));
		}
	}

	// Convenience method for initializing an array of 16-bit integers.
	template <typename T, size_t U>
	void initInt16Array(std::array<T, U> &arr, const char *data)
	{
		static_assert(sizeof(T) == 2, "sizeof(T) != 2.");

		for (size_t i = 0; i < arr.size(); i++)
		{
			arr.at(i) = static_cast<T>(
				Bytes::getLE16(reinterpret_cast<const uint8_t*>(data + (i * 2))));
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
		auto uniqueEnd = std::remove_copy(arr.begin(), arr.end(), uniqueBegin, 0);
		std::sort(uniqueBegin, uniqueEnd);
		uniqueEnd = std::unique(uniqueBegin, uniqueEnd);

		// For each offset, if it is non-zero, its position in the uniques array is
		// the index to put into the index array.
		for (size_t i = 0; i < arr.size(); i++)
		{
			const T offset = arr.at(i);
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

			indexArr.at(i) = index;
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

void ExeData::Calendar::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int monthNamesOffset = ExeData::get("MonthNames", keyValueMap);
	const int timesOfDayOffset = ExeData::get("TimesOfDay", keyValueMap);
	const int weekdayNamesOffset = ExeData::get("WeekdayNames", keyValueMap);

	initStringArray(this->monthNames, data + monthNamesOffset);
	initStringArray(this->timesOfDay, data + timesOfDayOffset);
	initStringArray(this->weekdayNames, data + weekdayNamesOffset);
}

void ExeData::CharacterClasses::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int allowedArmorsOffset = ExeData::get("AllowedArmors", keyValueMap);
	const int allowedShieldsOffset = ExeData::get("AllowedShields", keyValueMap);
	const int allowedShieldsListsOffset = ExeData::get("AllowedShieldsLists", keyValueMap);
	const int allowedWeaponsOffset = ExeData::get("AllowedWeapons", keyValueMap);
	const int allowedWeaponsListsOffset = ExeData::get("AllowedWeaponsLists", keyValueMap);
	const int classNamesOffset = ExeData::get("ClassNames", keyValueMap);
	const int classNumbersToIDsOffset = ExeData::get("ClassNumbersToIDs", keyValueMap);
	const int healthDiceOffset = ExeData::get("HealthDice", keyValueMap);
	const int initialExpCapsOffset = ExeData::get("InitialExperienceCaps", keyValueMap);
	const int lockpickingDivisorsOffset = ExeData::get("LockpickingDivisors", keyValueMap);
	const int preferredAttributesOffset = ExeData::get("PreferredAttributes", keyValueMap);

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

void ExeData::CharacterCreation::init(const char *data, const KeyValueMap &keyValueMap)
{
	const auto chooseClassCreationPair = ExeData::getPair("ChooseClassCreation", keyValueMap);
	const auto chooseClassCreationGeneratePair =
		ExeData::getPair("ChooseClassCreationGenerate", keyValueMap);
	const auto chooseClassCreationSelectPair =
		ExeData::getPair("ChooseClassCreationSelect", keyValueMap);
	const auto classQuestionsIntroPair = ExeData::getPair("ClassQuestionsIntro", keyValueMap);
	const auto suggestedClassPair = ExeData::getPair("SuggestedClass", keyValueMap);
	const auto chooseClassListPair = ExeData::getPair("ChooseClassList", keyValueMap);
	const auto chooseNamePair = ExeData::getPair("ChooseName", keyValueMap);
	const auto chooseGenderPair = ExeData::getPair("ChooseGender", keyValueMap);
	const auto chooseGenderMalePair = ExeData::getPair("ChooseGenderMale", keyValueMap);
	const auto chooseGenderFemalePair = ExeData::getPair("ChooseGenderFemale", keyValueMap);
	const auto chooseRacePair = ExeData::getPair("ChooseRace", keyValueMap);
	const auto confirmRacePair = ExeData::getPair("ConfirmRace", keyValueMap);
	const auto confirmedRace1Pair = ExeData::getPair("ConfirmedRace1", keyValueMap);
	const auto confirmedRace2Pair = ExeData::getPair("ConfirmedRace2", keyValueMap);
	const auto confirmedRace3Pair = ExeData::getPair("ConfirmedRace3", keyValueMap);
	const auto confirmedRace4Pair = ExeData::getPair("ConfirmedRace4", keyValueMap);
	const auto distributeClassPointsPair = ExeData::getPair("DistributeClassPoints", keyValueMap);
	const auto chooseAttributesPair = ExeData::getPair("ChooseAttributes", keyValueMap);
	const auto chooseAttributesSavePair = ExeData::getPair("ChooseAttributesSave", keyValueMap);
	const auto chooseAttributesRerollPair =
		ExeData::getPair("ChooseAttributesReroll", keyValueMap);
	const auto chooseAppearancePair = ExeData::getPair("ChooseAppearance", keyValueMap);

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

void ExeData::CityGeneration::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int coastalCityListOffset = ExeData::get("CoastalCityList", keyValueMap);
	const int cityTemplateFilenamesOffset = ExeData::get("CityTemplateFilenames", keyValueMap);
	const int startingPositionsOffset = ExeData::get("StartingPositions", keyValueMap);
	const int reservedBlockListsOffset = ExeData::get("ReservedBlockLists", keyValueMap);

	initInt8Array(this->coastalCityList, data + coastalCityListOffset);
	initStringArray(this->templateFilenames, data + cityTemplateFilenamesOffset);
	initInt8PairArray(this->startingPositions, data + startingPositionsOffset);

	const uint8_t blockTerminator = 0;
	initJaggedInt8Array(this->reservedBlockLists, blockTerminator,
		data + reservedBlockListsOffset);
}

void ExeData::Entities::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int creatureNamesOffset = ExeData::get("CreatureNames", keyValueMap);
	const int maleMainRaceAttrsOffset = ExeData::get("MaleMainRaceAttributes", keyValueMap);
	const int femaleMainRaceAttrsOffset = ExeData::get("FemaleMainRaceAttributes", keyValueMap);
	const int guardAttributesOffset = ExeData::get("GuardAttributes", keyValueMap);
	const int creatureAttributesOffset = ExeData::get("CreatureAttributes", keyValueMap);
	const int animationFilenamesOffset = ExeData::get("CreatureAnimationFilenames", keyValueMap);
	const int maleCitizenAnimFilenamesOffset =
		ExeData::get("MaleCitizenAnimationFilenames", keyValueMap);
	const int femaleCitizenAnimFilenamesOffset =
		ExeData::get("FemaleCitizenAnimationFilenames", keyValueMap);
	const int cfaFilenameChunksOffset = ExeData::get("CFAFilenameChunks", keyValueMap);
	const int cfaFilenameTemplatesOffset = ExeData::get("CFAFilenameTemplates", keyValueMap);
	const int cfaHumansWithWeaponAnimsOffset =
		ExeData::get("CFAHumansWithWeaponAnimations", keyValueMap);
	const int cfaWeaponAnimationsOffset = ExeData::get("CFAWeaponAnimations", keyValueMap);

	initStringArray(this->names, data + creatureNamesOffset);
	init2DInt8Array(this->maleMainRaceAttributes, data + maleMainRaceAttrsOffset);
	init2DInt8Array(this->femaleMainRaceAttributes, data + femaleMainRaceAttrsOffset);
	init2DInt8Array(this->guardAttributes, data + guardAttributesOffset);
	init2DInt8Array(this->creatureAttributes, data + creatureAttributesOffset);
	initStringArray(this->animationFilenames, data + animationFilenamesOffset);
	initStringArray(this->maleCitizenAnimationFilenames, data + maleCitizenAnimFilenamesOffset);
	initStringArray(this->femaleCitizenAnimationFilenames,
		data + femaleCitizenAnimFilenamesOffset);
	initStringArray(this->cfaFilenameChunks, data + cfaFilenameChunksOffset);
	initStringArray(this->cfaFilenameTemplates, data + cfaFilenameTemplatesOffset);
	initStringArray(this->cfaHumansWithWeaponAnimations, data + cfaHumansWithWeaponAnimsOffset);
	initStringArray(this->cfaWeaponAnimations, data + cfaWeaponAnimationsOffset);
}

void ExeData::Equipment::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int bodyPartNamesOffset = ExeData::get("BodyPartNames", keyValueMap);
	const int armorNamesOffset = ExeData::get("ArmorNames", keyValueMap);
	const int shieldNamesOffset = ExeData::get("ShieldNames", keyValueMap);
	const int weaponNamesOffset = ExeData::get("WeaponNames", keyValueMap);
	const int metalNamesOffset = ExeData::get("MetalNames", keyValueMap);
	const int weaponAnimFilenamesOffset = ExeData::get("WeaponAnimationFilenames", keyValueMap);

	initStringArray(this->bodyPartNames, data + bodyPartNamesOffset);
	initStringArray(this->armorNames, data + armorNamesOffset);
	initStringArray(this->shieldNames, data + shieldNamesOffset);
	initStringArray(this->weaponNames, data + weaponNamesOffset);
	initStringArray(this->metalNames, data + metalNamesOffset);
	initStringArray(this->weaponAnimationFilenames, data + weaponAnimFilenamesOffset);
}

void ExeData::Locations::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int provinceNamesOffset = ExeData::get("ProvinceNames", keyValueMap);
	const int charCreationProvinceNamesOffset =
		ExeData::get("CharCreationProvinceNames", keyValueMap);
	const int provinceImgFilenamesOffset = ExeData::get("ProvinceImgFilenames", keyValueMap);
	const int locationTypesOffset = ExeData::get("LocationTypes", keyValueMap);
	const int centerProvinceCityMifNameOffset = ExeData::get("CenterProvinceCityMifName", keyValueMap);
	const int startDungeonNameOffset = ExeData::get("StartDungeonName", keyValueMap);
	const int startDungeonMifNameOffset = ExeData::get("StartDungeonMifName", keyValueMap);
	const int finalDungeonMifNameOffset = ExeData::get("FinalDungeonMifName", keyValueMap);
	const int staffProvincesOffset = ExeData::get("StaffProvinces", keyValueMap);
	const int climatesOffset = ExeData::get("Climates", keyValueMap);
	const int weatherTableOffset = ExeData::get("WeatherTable", keyValueMap);
	const int climateSpeedTablesOffset = ExeData::get("ClimateSpeedTables", keyValueMap);
	const int weatherSpeedTablesOffset = ExeData::get("WeatherSpeedTables", keyValueMap);
	const int rulerTitlesOffset = ExeData::get("RulerTitles", keyValueMap);
	const int wildernessNormalBlocksOffset = ExeData::get("WildernessNormalBlocks", keyValueMap);
	const int wildernessVillageBlocksOffset = ExeData::get("WildernessVillageBlocks", keyValueMap);
	const int wildernessDungeonBlocksOffset = ExeData::get("WildernessDungeonBlocks", keyValueMap);
	const int wildernessInnBlocksOffset = ExeData::get("WildernessInnBlocks", keyValueMap);
	const int wildernessTempleBlocksOffset = ExeData::get("WildernessTempleBlocks", keyValueMap);

	// Each province name is null-terminated and 98 bytes apart.
	for (size_t i = 0; i < this->provinceNames.size(); i++)
	{
		this->provinceNames.at(i) = ExeData::readString(data + provinceNamesOffset + (i * 98));
	}

	initStringArray(this->charCreationProvinceNames, data + charCreationProvinceNamesOffset);
	initStringArray(this->provinceImgFilenames, data + provinceImgFilenamesOffset);
	initStringArray(this->locationTypes, data + locationTypesOffset);
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

	auto initWildBlockList = [](std::vector<uint8_t> &vec, const char *data)
	{
		// Each wilderness block list starts with the list size.
		vec.resize(static_cast<uint8_t>(*data));

		const uint8_t *listStart = reinterpret_cast<const uint8_t*>(data + 1);
		const uint8_t *listEnd = listStart + vec.size();
		std::copy(listStart, listEnd, vec.data());
	};

	initWildBlockList(this->wildernessNormalBlocks, data + wildernessNormalBlocksOffset);
	initWildBlockList(this->wildernessVillageBlocks, data + wildernessVillageBlocksOffset);
	initWildBlockList(this->wildernessDungeonBlocks, data + wildernessDungeonBlocksOffset);
	initWildBlockList(this->wildernessInnBlocks, data + wildernessInnBlocksOffset);
	initWildBlockList(this->wildernessTempleBlocks, data + wildernessTempleBlocksOffset);
}

void ExeData::Logbook::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int logbookIsEmptyOffset = ExeData::get("LogbookIsEmpty", keyValueMap);

	this->logbookIsEmpty = ExeData::readString(data + logbookIsEmptyOffset);
}

void ExeData::Meta::init(const char *data, const KeyValueMap &keyValueMap)
{
	this->dataSegmentOffset = ExeData::get("DataSegmentOffset", keyValueMap);
}

void ExeData::Races::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int singularNamesOffset = ExeData::get("RaceSingularNames", keyValueMap);
	const int pluralNamesOffset = ExeData::get("RacePluralNames", keyValueMap);

	initStringArray(this->singularNames, data + singularNamesOffset);
	initStringArray(this->pluralNames, data + pluralNamesOffset);
}

void ExeData::Status::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int popUpOffset = ExeData::get("StatusPopUp", keyValueMap);
	const int dateOffset = ExeData::get("StatusDate", keyValueMap);
	const int fortifyOffset = ExeData::get("StatusFortify", keyValueMap);
	const int diseaseOffset = ExeData::get("StatusDisease", keyValueMap);
	const int effectOffset = ExeData::get("StatusEffect", keyValueMap);
	const int effectsListOffset = ExeData::get("StatusEffectsList", keyValueMap);

	this->popUp = ExeData::readString(data + popUpOffset);
	this->date = ExeData::readString(data + dateOffset);
	this->fortify = ExeData::readString(data + fortifyOffset);
	this->disease = ExeData::readString(data + diseaseOffset);
	this->effect = ExeData::readString(data + effectOffset);
	initStringArray(this->effectsList, data + effectsListOffset);
}

void ExeData::Travel::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int locationFormatTextsOffset = ExeData::get("LocationFormatTexts", keyValueMap);
	const int dayPredictionOffset = ExeData::get("DayPrediction", keyValueMap);
	const int distancePredictionOffset = ExeData::get("DistancePrediction", keyValueMap);
	const int arrivalDatePredictionOffset = ExeData::get("ArrivalDatePrediction", keyValueMap);
	const int alreadyAtDestinationOffset = ExeData::get("AlreadyAtDestination", keyValueMap);
	const int noDestinationOffset = ExeData::get("NoDestination", keyValueMap);
	const int arrivalPopUpLocationOffset = ExeData::get("ArrivalPopUpLocation", keyValueMap);
	const int arrivalPopUpDateOffset = ExeData::get("ArrivalPopUpDate", keyValueMap);
	const int arrivalPopUpDaysOffset = ExeData::get("ArrivalPopUpDays", keyValueMap);
	const int arrivalCenterProvinceLocationOffset =
		ExeData::get("ArrivalCenterProvinceLocation", keyValueMap);
	const int searchTitleTextOffset = ExeData::get("SearchTitleText", keyValueMap);
	const int staffDungeonSplashesOffset = ExeData::get("StaffDungeonSplashes", keyValueMap);
	const int staffDungeonSplashIndicesOffset = ExeData::get("StaffDungeonSplashIndices", keyValueMap);

	initStringArray(this->locationFormatTexts, data + locationFormatTextsOffset);
	initStringArray(this->dayPrediction, data + dayPredictionOffset);
	this->distancePrediction = ExeData::readString(data + distancePredictionOffset);
	this->arrivalDatePrediction = ExeData::readString(data + arrivalDatePredictionOffset);
	this->alreadyAtDestination = ExeData::readString(data + alreadyAtDestinationOffset);
	this->noDestination = ExeData::readString(data + noDestinationOffset);
	this->arrivalPopUpLocation = ExeData::readString(data + arrivalPopUpLocationOffset);
	this->arrivalPopUpDate = ExeData::readString(data + arrivalPopUpDateOffset);
	this->arrivalPopUpDays = ExeData::readString(data + arrivalPopUpDaysOffset);
	this->arrivalCenterProvinceLocation =
		ExeData::readString(data + arrivalCenterProvinceLocationOffset);
	this->searchTitleText = ExeData::readString(data + searchTitleTextOffset);
	initStringArray(this->staffDungeonSplashes, data + staffDungeonSplashesOffset);
	initInt8Array(this->staffDungeonSplashIndices, data + staffDungeonSplashIndicesOffset);
}

void ExeData::UI::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int chooseClassListUIOffset = ExeData::get("ChooseClassListUI", keyValueMap);

	this->chooseClassListUI.init(data + chooseClassListUIOffset);
}

void ExeData::WallHeightTables::init(const char *data, const KeyValueMap &keyValueMap)
{
	const int box1aOffset = ExeData::get("Box1A", keyValueMap);
	const int box1bOffset = ExeData::get("Box1B", keyValueMap);
	const int box1cOffset = ExeData::get("Box1C", keyValueMap);
	const int box2aOffset = ExeData::get("Box2A", keyValueMap);
	const int box2bOffset = ExeData::get("Box2B", keyValueMap);
	const int box3aOffset = ExeData::get("Box3A", keyValueMap);
	const int box3bOffset = ExeData::get("Box3B", keyValueMap);
	const int box4Offset = ExeData::get("Box4", keyValueMap);

	initInt16Array(this->box1a, data + box1aOffset);
	initInt16Array(this->box1b, data + box1bOffset);
	initInt16Array(this->box1c, data + box1cOffset);
	initInt16Array(this->box2a, data + box2aOffset);
	initInt16Array(this->box2b, data + box2bOffset);
	initInt16Array(this->box3a, data + box3aOffset);
	initInt16Array(this->box3b, data + box3bOffset);
	initInt16Array(this->box4, data + box4Offset);
}

const std::string ExeData::FLOPPY_VERSION_EXE_FILENAME = "A.EXE";
const std::string ExeData::FLOPPY_VERSION_MAP_FILENAME = "data/text/aExeStrings.txt";
const std::string ExeData::CD_VERSION_EXE_FILENAME = "ACD.EXE";
const std::string ExeData::CD_VERSION_MAP_FILENAME = "data/text/acdExeStrings.txt";
const char ExeData::PAIR_SEPARATOR = ',';

int ExeData::get(const std::string &key, const KeyValueMap &keyValueMap)
{
	const std::string &valueStr = keyValueMap.getString(key);

	// Make sure the value only has an offset and isn't an offset + length pair.
	DebugAssert(valueStr.find(ExeData::PAIR_SEPARATOR) == std::string::npos,
		"\"" + key + "\" should only have an offset.");

	int offset;

	std::stringstream ss;
	ss << std::hex << valueStr;
	ss >> offset;
	return offset;
}

std::pair<int, int> ExeData::getPair(const std::string &key, const KeyValueMap &keyValueMap)
{
	const std::string &valueStr = keyValueMap.getString(key);

	// Make sure the value has a comma-separated offset + length pair.
	const std::vector<std::string> tokens = String::split(valueStr, ExeData::PAIR_SEPARATOR);
	DebugAssert(tokens.size() == 2, "\"" + key + "\" should have an offset and length.");

	const std::string &offsetStr = tokens.front();
	const std::string &lengthStr = tokens.at(1);
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

std::string ExeData::readString(const char *data, int length)
{
	return std::string(data, length);
}

std::string ExeData::readFixedString(const char *data, const std::pair<int, int> &pair)
{
	return ExeData::readString(data + pair.first, pair.second);
}

bool ExeData::isFloppyVersion() const
{
	return this->floppyVersion;
}

void ExeData::init(bool floppyVersion)
{
	// Load executable.
	const std::string &exeFilename = floppyVersion ?
		ExeData::FLOPPY_VERSION_EXE_FILENAME : ExeData::CD_VERSION_EXE_FILENAME;
	const ExeUnpacker exe(exeFilename);
	const char *exeDataPtr = reinterpret_cast<const char*>(exe.getData().data());

	// Load key-value map file.
	const std::string &mapFilename = floppyVersion ?
		ExeData::FLOPPY_VERSION_MAP_FILENAME : ExeData::CD_VERSION_MAP_FILENAME;
	const KeyValueMap keyValueMap(Platform::getBasePath() + mapFilename);

	// Initialize members with the executable mappings.
	this->calendar.init(exeDataPtr, keyValueMap);
	this->charClasses.init(exeDataPtr, keyValueMap);
	this->charCreation.init(exeDataPtr, keyValueMap);
	this->cityGen.init(exeDataPtr, keyValueMap);
	this->entities.init(exeDataPtr, keyValueMap);
	this->equipment.init(exeDataPtr, keyValueMap);
	this->locations.init(exeDataPtr, keyValueMap);
	this->logbook.init(exeDataPtr, keyValueMap);
	this->meta.init(exeDataPtr, keyValueMap);
	this->races.init(exeDataPtr, keyValueMap);
	this->status.init(exeDataPtr, keyValueMap);
	this->travel.init(exeDataPtr, keyValueMap);
	this->ui.init(exeDataPtr, keyValueMap);
	this->wallHeightTables.init(exeDataPtr, keyValueMap);

	this->floppyVersion = floppyVersion;
}
