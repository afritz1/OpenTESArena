#include "SkyStarDefinition.h"

#include "components/debug/Debug.h"

void SkyStarDefinition::SmallStar::init(uint8_t paletteIndex)
{
	this->paletteIndex = paletteIndex;
}

void SkyStarDefinition::LargeStar::init(ImageID imageID)
{
	this->imageID = imageID;
}

void SkyStarDefinition::init(Type type)
{
	this->type = type;
}

void SkyStarDefinition::initSmall(uint8_t paletteIndex)
{
	this->init(Type::Small);
	this->smallStar.init(paletteIndex);
}

void SkyStarDefinition::initLarge(ImageID imageID)
{
	this->init(Type::Large);
	this->largeStar.init(imageID);
}

SkyStarDefinition::Type SkyStarDefinition::getType() const
{
	return this->type;
}

const SkyStarDefinition::SmallStar &SkyStarDefinition::getSmallStar() const
{
	DebugAssert(this->type == Type::Small);
	return this->smallStar;
}

const SkyStarDefinition::LargeStar &SkyStarDefinition::getLargeStar() const
{
	DebugAssert(this->type == Type::Large);
	return this->largeStar;
}
