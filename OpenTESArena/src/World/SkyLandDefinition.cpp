#include "SkyLandDefinition.h"

#include "components/debug/Debug.h"

void SkyLandDefinition::init(const TextureBuilderIdGroup &textureBuilderIDs, double animSeconds,
	ShadingType shadingType)
{
	this->textureBuilderIDs = textureBuilderIDs;
	this->animSeconds = animSeconds;
	this->shadingType = shadingType;
}

void SkyLandDefinition::init(TextureBuilderID textureBuilderID, ShadingType shadingType)
{
	const TextureBuilderIdGroup textureBuilderIDs(textureBuilderID, 1);
	constexpr double animSeconds = 0.0;
	this->init(textureBuilderIDs, animSeconds, shadingType);
}

int SkyLandDefinition::getTextureCount() const
{
	return this->textureBuilderIDs.getCount();
}

TextureBuilderID SkyLandDefinition::getTextureBuilderID(int index) const
{
	return this->textureBuilderIDs.getID(index);
}

bool SkyLandDefinition::hasAnimation() const
{
	return this->textureBuilderIDs.getCount() > 1;
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
