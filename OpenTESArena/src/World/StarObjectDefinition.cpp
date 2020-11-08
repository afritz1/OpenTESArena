#include "StarObjectDefinition.h"

#include "components/debug/Debug.h"

void StarObjectDefinition::SmallStar::init(uint32_t colorARGB)
{
	this->colorARGB = colorARGB;
}

void StarObjectDefinition::LargeStar::init(ImageID imageID)
{
	this->imageID = imageID;
}

void StarObjectDefinition::init(Type type, Radians angleX, Radians angleY)
{
	this->type = type;
	this->angleX = angleX;
	this->angleY = angleY;
}

void StarObjectDefinition::initSmall(Radians angleX, Radians angleY, uint32_t colorARGB)
{
	this->init(Type::Small, angleX, angleY);
	this->smallStar.init(colorARGB);
}

void StarObjectDefinition::initLarge(Radians angleX, Radians angleY, ImageID imageID)
{
	this->init(Type::Large, angleX, angleY);
	this->largeStar.init(imageID);
}

Radians StarObjectDefinition::getAngleX() const
{
	return this->angleX;
}

Radians StarObjectDefinition::getAngleY() const
{
	return this->angleY;
}

StarObjectDefinition::Type StarObjectDefinition::getType() const
{
	return this->type;
}

const StarObjectDefinition::SmallStar &StarObjectDefinition::getSmallStar() const
{
	DebugAssert(this->type == Type::Small);
	return this->smallStar;
}

const StarObjectDefinition::LargeStar &StarObjectDefinition::getLargeStar() const
{
	DebugAssert(this->type == Type::Large);
	return this->largeStar;
}
