#include "SkySunDefinition.h"

void SkySunDefinition::init(TextureAssetReference &&textureAssetRef)
{
	this->textureAssetRef = std::move(textureAssetRef);
}

const TextureAssetReference &SkySunDefinition::getTextureAssetRef() const
{
	return this->textureAssetRef;
}
