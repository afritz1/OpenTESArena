#include "SkyAirDefinition.h"

void SkyAirDefinition::init(TextureAsset &&textureAsset)
{
	this->textureAsset = std::move(textureAsset);
}

const TextureAsset &SkyAirDefinition::getTextureAsset() const
{
	return this->textureAsset;
}
