#include <algorithm>
#include <optional>

#include "ArenaLocationUtils.h"
#include "ProvinceDefinition.h"
#include "../Assets/BinaryAssetLibrary.h"

#include "components/debug/Debug.h"

void ProvinceDefinition::init(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary)
{
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const CityDataFile &cityData = binaryAssetLibrary.getCityDataFile();
	const auto &provinceData = cityData.getProvinceData(provinceID);
	this->name = provinceData.name;
	this->globalX = provinceData.globalX;
	this->globalY = provinceData.globalY;
	this->globalW = provinceData.globalW;
	this->globalH = provinceData.globalH;
	this->raceID = provinceID;
	this->animatedDistantLand = provinceID == 3;

	auto canAddLocation = [](const ArenaLocationData &locationData)
	{
		// @todo: don't think this works for dungeons because they are renamed when set visible.
		//return locationData.name.size() > 0;
		return true;
	};

	auto tryAddCity = [this, &binaryAssetLibrary, &provinceData, &canAddLocation](int localCityID,
		int provinceID, bool coastal, bool premade, ArenaTypes::CityType type)
	{
		const auto &locationData = provinceData.getLocationData(localCityID);

		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initCity(localCityID, provinceID, coastal, premade, type, binaryAssetLibrary);
			this->locations.emplace_back(std::move(locationDef));
		}
	};

	auto tryAddDungeon = [this, &provinceData, &canAddLocation](int localDungeonID, int provinceID, const ArenaLocationData &locationData)
	{
		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initDungeon(localDungeonID, provinceID, locationData, provinceData);
			this->locations.emplace_back(std::move(locationDef));
		}
	};

	auto tryAddMainQuestDungeon = [this, &binaryAssetLibrary, &provinceData, &canAddLocation](
		const std::optional<int> &optLocalDungeonID, int provinceID,
		LocationMainQuestDungeonDefinitionType type, const ArenaLocationData &locationData)
	{
		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initMainQuestDungeon(optLocalDungeonID, provinceID, type, binaryAssetLibrary);
			this->locations.emplace_back(std::move(locationDef));
		}
	};

	const bool isCenterProvince = provinceID == ArenaLocationUtils::CENTER_PROVINCE_ID;
	const ExeDataCityGeneration &cityGen = binaryAssetLibrary.getExeData().cityGen;

	auto tryAddCities = [provinceID, &cityGen, &tryAddCity, isCenterProvince](
		const auto &locations, ArenaTypes::CityType type, int startID)
	{
		auto isCoastal = [provinceID, &cityGen](int localCityID)
		{
			const int globalCityID = ArenaLocationUtils::getGlobalCityID(localCityID, provinceID);
			return std::find(std::begin(cityGen.coastalCityList), std::end(cityGen.coastalCityList), globalCityID) != std::end(cityGen.coastalCityList);
		};

		for (size_t i = 0; i < std::size(locations); i++)
		{
			const auto &location = locations[i];
			const int localCityID = startID + static_cast<int>(i);
			const bool coastal = isCoastal(localCityID);
			const bool premade = isCenterProvince && (localCityID == 0);
			tryAddCity(localCityID, provinceID, coastal, premade, type);
		}
	};

	auto tryAddDungeons = [provinceID, &tryAddDungeon](const auto &locations)
	{
		for (size_t i = 0; i < std::size(locations); i++)
		{
			const auto &location = locations[i];

			const int localDungeonID = 2 + static_cast<int>(i);
			tryAddDungeon(localDungeonID, provinceID, location);
		}
	};

	tryAddCities(provinceData.cityStates, ArenaTypes::CityType::CityState, 0);
	tryAddCities(provinceData.towns, ArenaTypes::CityType::Town, static_cast<int>(std::size(provinceData.cityStates)));
	tryAddCities(provinceData.villages, ArenaTypes::CityType::Village, static_cast<int>(std::size(provinceData.cityStates) + std::size(provinceData.towns)));

	tryAddMainQuestDungeon(0, provinceID, LocationMainQuestDungeonDefinitionType::Staff, provinceData.secondDungeon);
	tryAddMainQuestDungeon(1, provinceID, LocationMainQuestDungeonDefinitionType::Map, provinceData.firstDungeon);

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
		tryAddMainQuestDungeon(std::nullopt, provinceID,
			LocationMainQuestDungeonDefinitionType::Start, startDungeonLocation);
	}
}

int ProvinceDefinition::getLocationCount() const
{
	return static_cast<int>(this->locations.size());
}

const LocationDefinition &ProvinceDefinition::getLocationDef(int index) const
{
	DebugAssertIndex(this->locations, index);
	return this->locations[index];
}

const std::string &ProvinceDefinition::getName() const
{
	return this->name;
}

Rect ProvinceDefinition::getGlobalRect() const
{
	return Rect(this->globalX, this->globalY, this->globalW, this->globalH);
}

int ProvinceDefinition::getRaceID() const
{
	return this->raceID;
}

bool ProvinceDefinition::hasAnimatedDistantLand() const
{
	return this->animatedDistantLand;
}

bool ProvinceDefinition::matches(const ProvinceDefinition &other) const
{
	// Can't have two different provinces with identical world map areas.
	return (this->globalX == other.globalX) && (this->globalY == other.globalY) &&
		(this->globalW == other.globalW) && (this->globalH == other.globalH);
}

bool ProvinceDefinition::tryGetLocationIndex(const LocationDefinition &locationDef,
	int *outLocationIndex) const
{
	for (int i = 0; i < this->getLocationCount(); i++)
	{
		const LocationDefinition &curLocationDef = this->getLocationDef(i);
		if (curLocationDef.matches(locationDef))
		{
			*outLocationIndex = i;
			return true;
		}
	}

	return false;
}
