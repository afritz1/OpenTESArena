#include "LocationDefinition.h"

void LocationDefinition::init(const CityDataFile::ProvinceData::LocationData &locationData)
{
	this->name = locationData.name;
	this->x = locationData.x;
	this->y = locationData.y;
	this->visibleByDefault = locationData.isVisible();
}

const std::string &LocationDefinition::getName() const
{
	return this->name;
}

int LocationDefinition::getScreenX() const
{
	return this->x;
}

int LocationDefinition::getScreenY() const
{
	return this->y;
}

bool LocationDefinition::isVisibleByDefault() const
{
	return this->visibleByDefault;
}
