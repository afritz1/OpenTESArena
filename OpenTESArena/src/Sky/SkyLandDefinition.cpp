#include "SkyLandDefinition.h"

void SkyLandDefinition::init(Buffer<TextureAsset> &&textureAssets, double animSeconds, SkyLandShadingType shadingType)
{
	this->textureAssets = std::move(textureAssets);
	this->animSeconds = animSeconds;
	this->hasAnimation = this->textureAssets.getCount() > 1;
	this->shadingType = shadingType;
}

void SkyLandDefinition::init(TextureAsset &&textureAsset, SkyLandShadingType shadingType)
{
	Buffer<TextureAsset> textureAssets(1);
	textureAssets.set(0, std::move(textureAsset));

	constexpr double animSeconds = 0.0;
	this->init(std::move(textureAssets), animSeconds, shadingType);
}
