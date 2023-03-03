#include "SkySunDefinition.h"

void SkySunDefinition::init(TextureAsset &&textureAsset)
{
	this->textureAsset = std::move(textureAsset);
}

const TextureAsset &SkySunDefinition::getTextureAsset() const
{
	return this->textureAsset;
}
