#include "SkyAirDefinition.h"

void SkyAirDefinition::init(TextureAssetReference &&textureAssetRef)
{
	this->textureAssetRef = std::move(textureAssetRef);
}

const TextureAssetReference &SkyAirDefinition::getTextureAssetRef() const
{
	return this->textureAssetRef;
}
