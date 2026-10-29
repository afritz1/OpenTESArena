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

	auto canAddLocation = [](const ArenaLocationData &locationData)
	{
		// @todo: don't think this works for dungeons because they are renamed when set visible.
		//return locationData.name.size() > 0;
		return true;
	};

	auto tryAddCity = [this, &binaryAssetLibrary, &provinceData, &canAddLocation](int localCityID,
		int originalProvinceIndex, bool coastal, bool premade, ArenaCityType type)
	{
		const ArenaLocationData &locationData = provinceData.getLocationData(localCityID);

		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initCity(localCityID, originalProvinceIndex, coastal, premade, type, binaryAssetLibrary);
			this->locations.emplace_back(std::move(locationDef));
		}
	};

	auto tryAddDungeon = [this, &provinceData, &canAddLocation](int localDungeonID, int originalProvinceIndex, const ArenaLocationData &locationData)
	{
		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initDungeon(localDungeonID, originalProvinceIndex, locationData, provinceData);
			this->locations.emplace_back(std::move(locationDef));
		}
	};

	auto tryAddMainQuestDungeon = [this, &binaryAssetLibrary, &provinceData, &canAddLocation](
		const std::optional<int> &optLocalDungeonID, int originalProvinceIndex,
		LocationMainQuestDungeonDefinitionType type, const ArenaLocationData &locationData)
	{
		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initMainQuestDungeon(optLocalDungeonID, originalProvinceIndex, type, binaryAssetLibrary);
			this->locations.emplace_back(std::move(locationDef));
		}
	};

	const bool isCenterProvince = originalProvinceIndex == ArenaLocationUtils::CENTER_PROVINCE_INDEX;
	const ExeDataCityGeneration &cityGen = binaryAssetLibrary.getExeData().cityGen;

	auto tryAddCities = [originalProvinceIndex, &cityGen, &tryAddCity, isCenterProvince](Span<const ArenaLocationData> locations, ArenaCityType type, int startID)
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
			tryAddCity(localCityID, originalProvinceIndex, coastal, premade, type);
		}
	};

	auto tryAddDungeons = [originalProvinceIndex, &tryAddDungeon](Span<const ArenaLocationData> locations)
	{
		for (size_t i = 0; i < std::size(locations); i++)
		{
			const ArenaLocationData &location = locations[i];
			const int localDungeonID = 2 + static_cast<int>(i);
			tryAddDungeon(localDungeonID, originalProvinceIndex, location);
		}
	};

	tryAddCities(provinceData.cityStates, ArenaCityType::CityState, 0);
	tryAddCities(provinceData.towns, ArenaCityType::Town, static_cast<int>(std::size(provinceData.cityStates)));
	tryAddCities(provinceData.villages, ArenaCityType::Village, static_cast<int>(std::size(provinceData.cityStates) + std::size(provinceData.towns)));

	tryAddMainQuestDungeon(0, originalProvinceIndex, LocationMainQuestDungeonDefinitionType::Staff, provinceData.secondDungeon);
	tryAddMainQuestDungeon(1, originalProvinceIndex, LocationMainQuestDungeonDefinitionType::Map, provinceData.firstDungeon);

	tryAddDungeons(provinceData.randomDungeons);

	const bool hasStartDungeon = isCenterProvince;
	if (hasStartDungeon)
	{
		ArenaLocationData startDungeonLocation;
		startDungeonLocation.name = std::string();
		startDungeonLocation.x = 0;
		startDungeonLocation.y = 0;
		startDungeonLocation.setVisible(false);

		// After main quest dungeons and regular dungeons (anywhere's fine in the new layout, I guess).
		tryAddMainQuestDungeon(std::nullopt, originalProvinceIndex, LocationMainQuestDungeonDefinitionType::Start, startDungeonLocation);
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
