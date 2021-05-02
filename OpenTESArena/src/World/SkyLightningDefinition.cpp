#include "SkyLightningDefinition.h"

void SkyLightningDefinition::init(Buffer<TextureAssetReference> &&textureAssetRefs, double animSeconds)
{
	this->textureAssetRefs = std::move(textureAssetRefs);
	this->animSeconds = animSeconds;
}

int SkyLightningDefinition::getTextureCount() const
{
	return static_cast<int>(this->textureAssetRefs.getCount());
}

const TextureAssetReference &SkyLightningDefinition::getTextureAssetRef(int index) const
{
	return this->textureAssetRefs.get(index);
}

double SkyLightningDefinition::getAnimationSeconds() const
{
	return this->animSeconds;
}
