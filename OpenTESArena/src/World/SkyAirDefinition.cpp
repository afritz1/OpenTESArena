#include "SkyAirDefinition.h"

void SkyAirDefinition::init(ImageID imageID)
{
	this->imageID = imageID;
}

ImageID SkyAirDefinition::getImageID() const
{
	return this->imageID;
}
