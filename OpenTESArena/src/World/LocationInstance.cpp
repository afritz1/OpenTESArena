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

void LocationInstance::toggleVisibility()
{
	this->visible = !this->visible;
}
