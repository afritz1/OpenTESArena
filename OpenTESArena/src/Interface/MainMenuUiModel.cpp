#include <numeric>

#include "MainMenuUiModel.h"
#include "../Assets/ExeData.h"
#include "../Math/RandomUtils.h"
#include "../WorldMap/LocationUtils.h"
#include "../WorldMap/ProvinceDefinition.h"

#include "components/debug/Debug.h"

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
	else
	{
		return "Dungeon";
	}
}

void MainMenuUiModel::getMainQuestLocationFromIndex(int testIndex, const ExeData &exeData, int *outLocationID,
	int *outProvinceID, SpecialCaseType *outSpecialCaseType)
{
	if (testIndex == 0)
	{
		*outLocationID = -1;
		*outProvinceID = LocationUtils::CENTER_PROVINCE_ID;
		*outSpecialCaseType = SpecialCaseType::StartDungeon;
	}
	else if (testIndex == (MainQuestLocationCount - 1))
	{
		*outLocationID = 0;
		*outProvinceID = LocationUtils::CENTER_PROVINCE_ID;
		*outSpecialCaseType = SpecialCaseType::None;
	}
	else
	{
		// Generate the location from the executable data.
		const auto &staffProvinces = exeData.locations.staffProvinces;
		const int staffProvincesIndex = (testIndex - 1) / 2;
		DebugAssertIndex(staffProvinces, staffProvincesIndex);
		*outProvinceID = staffProvinces[staffProvincesIndex];
		*outLocationID = LocationUtils::dungeonToLocationID(testIndex % 2);
		*outSpecialCaseType = SpecialCaseType::None;
	}
}

std::vector<int> MainMenuUiModel::makeShuffledLocationIndices(const ProvinceDefinition &provinceDef)
{
	std::vector<int> indices(provinceDef.getLocationCount());
	std::iota(indices.begin(), indices.end(), 0);
	RandomUtils::shuffle(indices.data(), static_cast<int>(indices.size()));
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
