#include "SkyLandDefinition.h"

#include "components/debug/Debug.h"

void SkyLandDefinition::init(Buffer<TextureAsset> &&textureAssets, double animSeconds,
	ShadingType shadingType)
{
	this->textureAssets = std::move(textureAssets);
	this->animSeconds = animSeconds;
	this->shadingType = shadingType;
}

void SkyLandDefinition::init(TextureAsset &&textureAsset, ShadingType shadingType)
{
	Buffer<TextureAsset> textureAssets(1);
	textureAssets.set(0, std::move(textureAsset));

	constexpr double animSeconds = 0.0;
	this->init(std::move(textureAssets), animSeconds, shadingType);
}

int SkyLandDefinition::getTextureCount() const
{
	return static_cast<int>(this->textureAssets.getCount());
}

const TextureAsset &SkyLandDefinition::getTextureAsset(int index) const
{
	return this->textureAssets.get(index);
}

bool SkyLandDefinition::hasAnimation() const
{
	return this->getTextureCount() > 1;
}

double SkyLandDefinition::getAnimationSeconds() const
{
	DebugAssert(this->hasAnimation());
	return this->animSeconds;
}

SkyLandDefinition::ShadingType SkyLandDefinition::getShadingType() const
{
	return this->shadingType;
}
