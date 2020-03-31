#include "ProvinceDefinition.h"

#include "components/debug/Debug.h"

void ProvinceDefinition::init(const CityDataFile::ProvinceData &provinceData)
{
	this->name = provinceData.name;
	this->globalX = provinceData.globalX;
	this->globalY = provinceData.globalY;
	this->globalW = provinceData.globalW;
	this->globalH = provinceData.globalH;

	auto tryAddLocationDef = [this](const CityDataFile::ProvinceData::LocationData &locationData)
	{
		if (locationData.name.size() > 0)
		{
			LocationDefinition locationDef;
			locationDef.init(locationData);
			this->locations.push_back(std::move(locationDef));
		}
	};

	auto tryAddLocationDefs = [&tryAddLocationDef](const auto &locations)
	{
		for (const CityDataFile::ProvinceData::LocationData &locationData : locations)
		{
			tryAddLocationDef(locationData);
		}
	};

	tryAddLocationDefs(provinceData.cityStates);
	tryAddLocationDefs(provinceData.towns);
	tryAddLocationDefs(provinceData.villages);

	tryAddLocationDef(provinceData.secondDungeon);
	tryAddLocationDef(provinceData.firstDungeon);

	tryAddLocationDefs(provinceData.randomDungeons);
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
