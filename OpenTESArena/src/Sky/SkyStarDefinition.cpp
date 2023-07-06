#include "SkyStarDefinition.h"

#include "components/debug/Debug.h"

void SkySmallStarDefinition::init(uint8_t paletteIndex)
{
	this->paletteIndex = paletteIndex;
}

void SkyLargeStarDefinition::init(TextureAsset &&textureAsset)
{
	this->textureAsset = std::move(textureAsset);
}

void SkyStarDefinition::init(SkyStarType type)
{
	this->type = type;
}

void SkyStarDefinition::initSmall(uint8_t paletteIndex)
{
	this->init(SkyStarType::Small);
	this->smallStar.init(paletteIndex);
}

void SkyStarDefinition::initLarge(TextureAsset &&textureAsset)
{
	this->init(SkyStarType::Large);
	this->largeStar.init(std::move(textureAsset));
}

SkyStarType SkyStarDefinition::getType() const
{
	return this->type;
}

const SkySmallStarDefinition &SkyStarDefinition::getSmallStar() const
{
	DebugAssert(this->type == SkyStarType::Small);
	return this->smallStar;
}

const SkyLargeStarDefinition &SkyStarDefinition::getLargeStar() const
{
	DebugAssert(this->type == SkyStarType::Large);
	return this->largeStar;
}
