#include "LandObjectDefinition.h"

#include "components/debug/Debug.h"

void LandObjectDefinition::init(const TextureManager::IdGroup<ImageID> &imageIDs, double animSeconds,
	ShadingType shadingType)
{
	this->imageIDs = imageIDs;
	this->animSeconds = animSeconds;
	this->shadingType = shadingType;
}

void LandObjectDefinition::init(ImageID imageID, ShadingType shadingType)
{
	TextureManager::IdGroup<ImageID> imageIDs(imageID, 1);
	constexpr double animSeconds = 0.0;
	this->init(imageIDs, animSeconds, shadingType);
}

int LandObjectDefinition::getImageCount() const
{
	return this->imageIDs.getCount();
}

ImageID LandObjectDefinition::getImageID(int index) const
{
	return this->imageIDs.getID(index);
}

bool LandObjectDefinition::hasAnimation() const
{
	return imageIDs.getCount() > 1;
}

double LandObjectDefinition::getAnimationSeconds() const
{
	DebugAssert(this->hasAnimation());
	return this->animSeconds;
}

LandObjectDefinition::ShadingType LandObjectDefinition::getShadingType() const
{
	return this->shadingType;
}
