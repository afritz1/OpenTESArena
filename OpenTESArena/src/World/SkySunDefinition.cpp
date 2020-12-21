#include "SkySunDefinition.h"

void SkySunDefinition::init(ImageID imageID)
{
	this->imageID = imageID;
}

ImageID SkySunDefinition::getImageID() const
{
	return this->imageID;
}
