#include "AirObjectDefinition.h"

void AirObjectDefinition::init(ImageID imageID)
{
	this->imageID = imageID;
}

ImageID AirObjectDefinition::getImageID() const
{
	return this->imageID;
}
