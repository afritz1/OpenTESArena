#include <numeric>

#include "MainMenuUiModel.h"
#include "../Assets/ExeData.h"
#include "../Game/Game.h"
#include "../Math/RandomUtils.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"
#include "../WorldMap/ProvinceDefinition.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string MainMenuUiModel::getTestButtonText()
{
	return "Test";
}

std::string MainMenuUiModel::getTestTypeName(int type)
{
	if (type == TestType_MainQuest)
	{
		return "Main Quest";
	}
	else if (type == TestType_Interior)
	{
		return "Interior";
	}
	else if (type == TestType_City)
	{
		return "City";
	}
	else if (type == TestType_Wilderness)
	{
		return "Wilderness";
	}
	else if (type == TestType_Dungeon)
	{
		return "Dungeon";
	}
	else
	{
		DebugUnhandledReturnMsg(std::string, std::to_string(type));
	}
}

std::string MainMenuUiModel::getSelectedTestName(Game &game, int testType, int testIndex, int testIndex2)
{
	if (testType == MainMenuUiModel::TestType_MainQuest)
	{
		const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const auto &exeData = binaryAssetLibrary.getExeData();

		// Decide how to get the main quest dungeon name.
		if (testIndex == 0)
		{
			// Start dungeon.
			return String::toUppercase(exeData.locations.startDungeonMifName);
		}
		else if (testIndex == (MainMenuUiModel::MainQuestLocationCount - 1))
		{
			// Final dungeon.
			return String::toUppercase(exeData.locations.finalDungeonMifName);
		}
		else
		{
			// Generate the location from the executable data, fetching data from a
			// global function.
			int locationID, provinceID;
			MainMenuUiModel::SpecialCaseType specialCaseType;
			MainMenuUiModel::getMainQuestLocationFromIndex(testIndex, exeData, &locationID, &provinceID, &specialCaseType);
			DebugAssert(specialCaseType == MainMenuUiModel::SpecialCaseType::None);

			// Calculate the .MIF name from the dungeon seed.
			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const uint32_t dungeonSeed = [&cityData, locationID, provinceID]()
			{
				const auto &province = cityData.getProvinceData(provinceID);
				const int localDungeonID = locationID - 32;
				return ArenaLocationUtils::getDungeonSeed(localDungeonID, provinceID, province);
			}();

			const std::string mifName = ArenaLocationUtils::getMainQuestDungeonMifName(dungeonSeed);
			return String::toUppercase(mifName);
		}
	}
	else if (testType == MainMenuUiModel::TestType_Interior)
	{
		const auto &interior = MainMenuUiModel::InteriorLocations.at(testIndex);
		return std::get<0>(interior) + std::to_string(testIndex2) + ".MIF";
	}
	else if (testType == MainMenuUiModel::TestType_City)
	{
		return MainMenuUiModel::CityLocations.at(testIndex);
	}
	else if (testType == MainMenuUiModel::TestType_Wilderness)
	{
		return MainMenuUiModel::WildernessLocations.at(testIndex);
	}
	else if (testType == MainMenuUiModel::TestType_Dungeon)
	{
		return MainMenuUiModel::DungeonLocations.at(testIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(std::string, std::to_string(testType));
	}
}

std::optional<ArenaTypes::InteriorType> MainMenuUiModel::getSelectedTestInteriorType(int testType, int testIndex)
{
	if ((testType == MainMenuUiModel::TestType_MainQuest) || (testType == MainMenuUiModel::TestType_Dungeon))
	{
		return ArenaTypes::InteriorType::Dungeon;
	}
	else if (testType == MainMenuUiModel::TestType_Interior)
	{
		DebugAssertIndex(MainMenuUiModel::InteriorLocations, testIndex);
		const auto &interior = MainMenuUiModel::InteriorLocations[testIndex];
		return std::get<2>(interior);
	}
	else if ((testType == MainMenuUiModel::TestType_City) || (testType == MainMenuUiModel::TestType_Wilderness))
	{
		return std::nullopt;
	}
	else
	{
		DebugUnhandledReturnMsg(std::optional<ArenaTypes::InteriorType>, std::to_string(testType));
	}
}

ArenaTypes::WeatherType MainMenuUiModel::getSelectedTestWeatherType(int testWeather)
{
	DebugAssertIndex(MainMenuUiModel::Weathers, testWeather);
	return MainMenuUiModel::Weathers[testWeather];
}

MapType MainMenuUiModel::getSelectedTestMapType(int testType)
{
	if ((testType == MainMenuUiModel::TestType_MainQuest) ||
		(testType == MainMenuUiModel::TestType_Interior) ||
		(testType == MainMenuUiModel::TestType_Dungeon))
	{
		return MapType::Interior;
	}
	else if (testType == MainMenuUiModel::TestType_City)
	{
		return MapType::City;
	}
	else if (testType == MainMenuUiModel::TestType_Wilderness)
	{
		return MapType::Wilderness;
	}
	else
	{
		DebugUnhandledReturnMsg(MapType, std::to_string(testType));
	}
}

void MainMenuUiModel::getMainQuestLocationFromIndex(int testIndex, const ExeData &exeData, int *outLocationID,
	int *outProvinceID, SpecialCaseType *outSpecialCaseType)
{
	if (testIndex == 0)
	{
		*outLocationID = -1;
		*outProvinceID = ArenaLocationUtils::CENTER_PROVINCE_ID;
		*outSpecialCaseType = SpecialCaseType::StartDungeon;
	}
	else if (testIndex == (MainQuestLocationCount - 1))
	{
		*outLocationID = 0;
		*outProvinceID = ArenaLocationUtils::CENTER_PROVINCE_ID;
		*outSpecialCaseType = SpecialCaseType::None;
	}
	else
	{
		// Generate the location from the executable data.
		const auto &staffProvinces = exeData.locations.staffProvinces;
		const int staffProvincesIndex = (testIndex - 1) / 2;
		DebugAssertIndex(staffProvinces, staffProvincesIndex);
		*outProvinceID = staffProvinces[staffProvincesIndex];
		*outLocationID = ArenaLocationUtils::dungeonToLocationID(testIndex % 2);
		*outSpecialCaseType = SpecialCaseType::None;
	}
}

std::vector<int> MainMenuUiModel::makeShuffledLocationIndices(const ProvinceDefinition &provinceDef)
{
	std::vector<int> indices(provinceDef.getLocationCount());
	std::iota(indices.begin(), indices.end(), 0);
	RandomUtils::shuffle(indices);
	return indices;
}

std::optional<int> MainMenuUiModel::getRandomCityLocationDefIndexIfType(const ProvinceDefinition &provinceDef,
	ArenaTypes::CityType cityType)
{
	// Iterate over locations in the province in a random order.
	const std::vector<int> randomLocationIndices = MainMenuUiModel::makeShuffledLocationIndices(provinceDef);

	for (const int locationIndex : randomLocationIndices)
	{
		const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
		if (curLocationDef.getType() == LocationDefinition::Type::City)
		{
			const auto &curCityDef = curLocationDef.getCityDefinition();
			if (curCityDef.type == cityType)
			{
				return locationIndex;
			}
		}
	}

	return std::nullopt;
}

int MainMenuUiModel::getRandomCityLocationIndex(const ProvinceDefinition &provinceDef)
{
	// Iterate over locations in the province in a random order.
	const std::vector<int> randomLocationIndices = MainMenuUiModel::makeShuffledLocationIndices(provinceDef);

	for (const int locationIndex : randomLocationIndices)
	{
		const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
		if (curLocationDef.getType() == LocationDefinition::Type::City)
		{
			return locationIndex;
		}
	}

	return -1;
}

std::optional<int> MainMenuUiModel::getRandomDungeonLocationDefIndex(const ProvinceDefinition &provinceDef)
{
	// Iterate over locations in the province in a random order.
	const std::vector<int> randomLocationIndices = MainMenuUiModel::makeShuffledLocationIndices(provinceDef);

	for (const int locationIndex : randomLocationIndices)
	{
		const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
		if (curLocationDef.getType() == LocationDefinition::Type::Dungeon)
		{
			return locationIndex;
		}
	}

	return std::nullopt;
}
