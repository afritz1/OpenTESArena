#include <algorithm>
#include <optional>

#include "ArenaLocationUtils.h"
#include "ProvinceDefinition.h"
#include "../Assets/BinaryAssetLibrary.h"

#include "components/debug/Debug.h"

void ProvinceDefinition::init(int originalProvinceIndex, const BinaryAssetLibrary &binaryAssetLibrary)
{
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const CityDataFile &cityData = binaryAssetLibrary.getCityDataFile();
	const auto &provinceData = cityData.getProvinceData(originalProvinceIndex);
	this->name = provinceData.name;
	this->globalX = provinceData.globalX;
	this->globalY = provinceData.globalY;
	this->globalW = provinceData.globalW;
	this->globalH = provinceData.globalH;
	this->originalProvinceIndex = originalProvinceIndex;
	this->hasAnimatedDistantLand = originalProvinceIndex == 3;

	auto addCity = [this, &binaryAssetLibrary](int localCityID, int originalProvinceIndex, bool coastal, bool premade, ArenaCityType type)
	{
		LocationDefinition locationDef;
		locationDef.initCity(localCityID, originalProvinceIndex, coastal, premade, type, binaryAssetLibrary);
		this->locations.emplace_back(std::move(locationDef));
	};

	auto addDungeon = [this, &provinceData](int localDungeonID, int originalProvinceIndex, const ArenaLocationData &locationData)
	{
		LocationDefinition locationDef;
		locationDef.initDungeon(localDungeonID, originalProvinceIndex, locationData, provinceData);
		this->locations.emplace_back(std::move(locationDef));
	};

	auto addMainQuestDungeon = [this, &binaryAssetLibrary](const std::optional<int> &optLocalDungeonID,
		int originalProvinceIndex, LocationMainQuestDungeonDefinitionType type)
	{
		LocationDefinition locationDef;
		locationDef.initMainQuestDungeon(optLocalDungeonID, originalProvinceIndex, type, binaryAssetLibrary);
		this->locations.emplace_back(std::move(locationDef));
	};

	const bool isCenterProvince = originalProvinceIndex == ArenaLocationUtils::CENTER_PROVINCE_INDEX;
	const ExeDataCityGeneration &cityGen = binaryAssetLibrary.getExeData().cityGen;

	auto addCities = [originalProvinceIndex, &cityGen, &addCity, isCenterProvince](Span<const ArenaLocationData> locations, ArenaCityType type, int startID)
	{
		auto isCoastal = [originalProvinceIndex, &cityGen](int localCityID)
		{
			const int globalCityID = ArenaLocationUtils::getGlobalCityID(localCityID, originalProvinceIndex);
			return std::find(std::begin(cityGen.coastalCityList), std::end(cityGen.coastalCityList), globalCityID) != std::end(cityGen.coastalCityList);
		};

		for (size_t i = 0; i < std::size(locations); i++)
		{
			const ArenaLocationData &location = locations[i];
			const int localCityID = startID + static_cast<int>(i);
			const bool coastal = isCoastal(localCityID);
			const bool premade = isCenterProvince && (localCityID == 0);
			addCity(localCityID, originalProvinceIndex, coastal, premade, type);
		}
	};

	auto addDungeons = [originalProvinceIndex, &addDungeon](Span<const ArenaLocationData> locations)
	{
		for (size_t i = 0; i < std::size(locations); i++)
		{
			const ArenaLocationData &location = locations[i];
			const int localDungeonID = 2 + static_cast<int>(i);
			addDungeon(localDungeonID, originalProvinceIndex, location);
		}
	};

	addCities(provinceData.cityStates, ArenaCityType::CityState, 0);
	addCities(provinceData.towns, ArenaCityType::Town, static_cast<int>(std::size(provinceData.cityStates)));
	addCities(provinceData.villages, ArenaCityType::Village, static_cast<int>(std::size(provinceData.cityStates) + std::size(provinceData.towns)));

	addMainQuestDungeon(0, originalProvinceIndex, LocationMainQuestDungeonDefinitionType::Staff);
	addMainQuestDungeon(1, originalProvinceIndex, LocationMainQuestDungeonDefinitionType::Map);

	addDungeons(provinceData.randomDungeons);

	const bool hasStartDungeon = isCenterProvince;
	if (hasStartDungeon)
	{
		// After main quest dungeons and regular dungeons (anywhere's fine in the new layout, I guess).
		addMainQuestDungeon(std::nullopt, originalProvinceIndex, LocationMainQuestDungeonDefinitionType::Start);
	}
}

Rect ProvinceDefinition::getGlobalRect() const
{
	return Rect(this->globalX, this->globalY, this->globalW, this->globalH);
}

bool ProvinceDefinition::matches(const ProvinceDefinition &other) const
{
	// Can't have two different provinces with identical world map areas.
	return (this->globalX == other.globalX) && (this->globalY == other.globalY) &&
		(this->globalW == other.globalW) && (this->globalH == other.globalH);
}

int ProvinceDefinition::findLocationIndexIf(const ProvinceLocationPredicate &predicate) const
{
	for (int i = 0; i < static_cast<int>(this->locations.size()); i++)
	{
		const LocationDefinition &locationDef = this->locations[i];
		if (predicate(locationDef))
		{
			return i;
		}
	}

	return -1;
}
