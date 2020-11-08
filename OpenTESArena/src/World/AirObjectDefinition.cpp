#include "AirObjectDefinition.h"

void AirObjectDefinition::init(Radians angleX, Radians angleY, ImageID imageID)
{
	this->angleX = angleX;
	this->angleY = angleY;
	this->imageID = imageID;
}

Radians AirObjectDefinition::getAngleX() const
{
	return this->angleX;
}

Radians AirObjectDefinition::getAngleY() const
{
	return this->angleY;
}

ImageID AirObjectDefinition::getImageID() const
{
	return this->imageID;
}
