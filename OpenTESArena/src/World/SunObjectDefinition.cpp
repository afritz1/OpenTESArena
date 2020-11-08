#include "SunObjectDefinition.h"

void SunObjectDefinition::init(double bonusLatitude, ImageID imageID)
{
	this->bonusLatitude = bonusLatitude;
	this->imageID = imageID;
}

double SunObjectDefinition::getBonusLatitude() const
{
	return this->bonusLatitude;
}

ImageID SunObjectDefinition::getImageID() const
{
	return this->imageID;
}
