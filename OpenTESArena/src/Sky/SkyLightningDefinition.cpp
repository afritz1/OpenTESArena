#include "SkyLightningDefinition.h"

void SkyLightningDefinition::init(Buffer<TextureAsset> &&textureAssets, double animSeconds)
{
	this->textureAssets = std::move(textureAssets);
	this->animSeconds = animSeconds;
}
