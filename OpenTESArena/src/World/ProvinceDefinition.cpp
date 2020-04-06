#include <algorithm>

#include "Location.h"
#include "ProvinceDefinition.h"
#include "../Assets/MiscAssets.h"

#include "components/debug/Debug.h"

void ProvinceDefinition::init(int provinceID, const MiscAssets &miscAssets)
{
	const ExeData &exeData = miscAssets.getExeData();
	const CityDataFile &cityData = miscAssets.getCityDataFile();
	const auto &provinceData = cityData.getProvinceData(provinceID);
	this->name = provinceData.name;
	this->globalX = provinceData.globalX;
	this->globalY = provinceData.globalY;
	this->globalW = provinceData.globalW;
	this->globalH = provinceData.globalH;

	auto canAddLocation = [](const CityDataFile::ProvinceData::LocationData &locationData)
	{
		// @todo: don't think this works for dungeons because they are renamed when set visible.
		//return locationData.name.size() > 0;
		return true;
	};

	auto tryAddCity = [this, &miscAssets, &provinceData, &canAddLocation](int localCityID,
		int provinceID, bool coastal, bool premade,
		LocationDefinition::CityDefinition::Type type, const MiscAssets &miscAssets)
	{
		const auto &locationData = provinceData.getLocationData(localCityID);

		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initCity(localCityID, provinceID, coastal, premade, type, miscAssets);
			this->locations.push_back(std::move(locationDef));
		}
	};

	auto tryAddDungeon = [this, &miscAssets, &canAddLocation](
		const CityDataFile::ProvinceData::LocationData &locationData)
	{
		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initDungeon(locationData);
			this->locations.push_back(std::move(locationDef));
		}
	};

	auto tryAddMainQuestDungeon = [this, &miscAssets, &exeData, &canAddLocation](
		LocationDefinition::MainQuestDungeonDefinition::Type type,
		const CityDataFile::ProvinceData::LocationData &locationData)
	{
		if (canAddLocation(locationData))
		{
			LocationDefinition locationDef;
			locationDef.initMainQuestDungeon(type, locationData, exeData);
			this->locations.push_back(std::move(locationDef));
		}
	};

	const bool isCenterProvince = provinceID == Location::CENTER_PROVINCE_ID;
	const ExeData::CityGeneration &cityGen = miscAssets.getExeData().cityGen;

	auto tryAddCities = [provinceID, &miscAssets, &cityGen, &tryAddCity, isCenterProvince](
		const auto &locations, LocationDefinition::CityDefinition::Type type, int startID)
	{
		auto isCoastal = [provinceID, &cityGen](int localCityID)
		{
			const int globalCityID = CityDataFile::getGlobalCityID(localCityID, provinceID);
			return std::find(cityGen.coastalCityList.begin(),
				cityGen.coastalCityList.end(), globalCityID) != cityGen.coastalCityList.end();
		};

		for (size_t i = 0; i < locations.size(); i++)
		{
			const auto &location = locations[i];
			const int localCityID = startID + static_cast<int>(i);
			const bool coastal = isCoastal(localCityID);
			const bool premade = isCenterProvince && (localCityID == 0);
			tryAddCity(localCityID, provinceID, coastal, premade, type, miscAssets);
		}
	};

	auto tryAddDungeons = [provinceID, &miscAssets, &tryAddDungeon](const auto &locations)
	{
		for (size_t i = 0; i < locations.size(); i++)
		{
			const auto &location = locations[i];
			tryAddDungeon(location);
		}
	};

	tryAddCities(provinceData.cityStates, LocationDefinition::CityDefinition::Type::City, 0);
	tryAddCities(provinceData.towns, LocationDefinition::CityDefinition::Type::Town,
		static_cast<int>(provinceData.cityStates.size()));
	tryAddCities(provinceData.villages, LocationDefinition::CityDefinition::Type::Village,
		static_cast<int>(provinceData.cityStates.size() + provinceData.towns.size()));

	tryAddMainQuestDungeon(LocationDefinition::MainQuestDungeonDefinition::Type::Staff,
		provinceData.secondDungeon);
	tryAddMainQuestDungeon(LocationDefinition::MainQuestDungeonDefinition::Type::Map,
		provinceData.firstDungeon);

	tryAddDungeons(provinceData.randomDungeons);

	const bool hasStartDungeon = isCenterProvince;
	if (hasStartDungeon)
	{
		CityDataFile::ProvinceData::LocationData startDungeonLocation;
		startDungeonLocation.name = std::string();
		startDungeonLocation.x = 0;
		startDungeonLocation.y = 0;
		startDungeonLocation.setVisible(false);

		// After main quest dungeons and regular dungeons.
		tryAddMainQuestDungeon(LocationDefinition::MainQuestDungeonDefinition::Type::Start,
			startDungeonLocation);
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
