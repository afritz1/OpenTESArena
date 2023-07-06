#include "SkyStarDefinition.h"

void SkySmallStarDefinition::init(uint8_t paletteIndex)
{
	this->paletteIndex = paletteIndex;
}

void SkyLargeStarDefinition::init(TextureAsset &&textureAsset)
{
	this->textureAsset = std::move(textureAsset);
}

void SkyStarDefinition::initSmall(uint8_t paletteIndex)
{
	this->type = SkyStarType::Small;
	this->smallStar.init(paletteIndex);
}

void SkyStarDefinition::initLarge(TextureAsset &&textureAsset)
{
	this->type = SkyStarType::Large;
	this->largeStar.init(std::move(textureAsset));
}
