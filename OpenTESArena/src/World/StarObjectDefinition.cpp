#include "StarObjectDefinition.h"

#include "components/debug/Debug.h"

void StarObjectDefinition::SmallStar::init(uint8_t paletteIndex)
{
	this->paletteIndex = paletteIndex;
}

void StarObjectDefinition::LargeStar::init(ImageID imageID)
{
	this->imageID = imageID;
}

void StarObjectDefinition::init(Type type)
{
	this->type = type;
}

void StarObjectDefinition::initSmall(uint8_t paletteIndex)
{
	this->init(Type::Small);
	this->smallStar.init(paletteIndex);
}

void StarObjectDefinition::initLarge(ImageID imageID)
{
	this->init(Type::Large);
	this->largeStar.init(imageID);
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
