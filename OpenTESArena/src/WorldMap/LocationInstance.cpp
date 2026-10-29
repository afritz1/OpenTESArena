#include "LocationInstance.h"

void LocationInstance::init(int locationDefIndex, const LocationDefinition &locationDef)
{
	this->locationDefIndex = locationDefIndex;
	this->visible = locationDef.isVisibleByDefault();
}

const std::string &LocationInstance::getName(const LocationDefinition &locationDef) const
{
	return !this->nameOverride.empty() ? this->nameOverride : locationDef.getName();
}
