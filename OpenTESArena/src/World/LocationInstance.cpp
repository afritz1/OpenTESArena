#include "LocationInstance.h"

void LocationInstance::init(int locationDefIndex, const LocationDefinition &locationDef)
{
	this->locationDefIndex = locationDefIndex;
	this->visible = locationDef.isVisibleByDefault();
}

int LocationInstance::getLocationDefIndex() const
{
	return this->locationDefIndex;
}

bool LocationInstance::isVisible() const
{
	return this->visible;
}

bool LocationInstance::hasNameOverride() const
{
	return this->nameOverride.size() > 0;
}

const std::string &LocationInstance::getName(const LocationDefinition &locationDef) const
{
	return this->hasNameOverride() ? this->nameOverride : locationDef.getName();
}

void LocationInstance::toggleVisibility()
{
	this->visible = !this->visible;
}

void LocationInstance::setNameOverride(std::string &&nameOverride)
{
	this->nameOverride = std::move(nameOverride);
}
