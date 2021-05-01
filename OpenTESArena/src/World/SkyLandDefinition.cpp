#include "SkyLandDefinition.h"

#include "components/debug/Debug.h"

void SkyLandDefinition::init(Buffer<TextureAssetReference> &&textureAssetRefs, double animSeconds,
	ShadingType shadingType)
{
	this->textureAssetRefs = std::move(textureAssetRefs);
	this->animSeconds = animSeconds;
	this->shadingType = shadingType;
}

void SkyLandDefinition::init(TextureAssetReference &&textureAssetRef, ShadingType shadingType)
{
	Buffer<TextureAssetReference> textureAssetRefs(1);
	textureAssetRefs.set(0, std::move(textureAssetRef));

	constexpr double animSeconds = 0.0;
	this->init(std::move(textureAssetRefs), animSeconds, shadingType);
}

int SkyLandDefinition::getTextureCount() const
{
	return static_cast<int>(this->textureAssetRefs.getCount());
}

const TextureAssetReference &SkyLandDefinition::getTextureAssetRef(int index) const
{
	return this->textureAssetRefs.get(index);
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
