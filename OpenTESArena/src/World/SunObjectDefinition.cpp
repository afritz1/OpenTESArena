#include "SunObjectDefinition.h"

void SunObjectDefinition::init(ImageID imageID)
{
	this->imageID = imageID;
}

ImageID SunObjectDefinition::getImageID() const
{
	return this->imageID;
}
