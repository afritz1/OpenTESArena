#include "SkyLightningDefinition.h"

void SkyLightningDefinition::init(Buffer<TextureAsset> &&textureAssets, double animSeconds)
{
	this->textureAssets = std::move(textureAssets);
	this->animSeconds = animSeconds;
}

int SkyLightningDefinition::getTextureCount() const
{
	return static_cast<int>(this->textureAssets.getCount());
}

const TextureAsset &SkyLightningDefinition::getTextureAsset(int index) const
{
	return this->textureAssets.get(index);
}

double SkyLightningDefinition::getAnimationSeconds() const
{
	return this->animSeconds;
}
