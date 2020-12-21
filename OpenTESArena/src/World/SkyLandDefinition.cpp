#include "SkyLandDefinition.h"

#include "components/debug/Debug.h"

void SkyLandDefinition::init(const TextureUtils::ImageIdGroup &imageIDs, double animSeconds,
	ShadingType shadingType)
{
	this->imageIDs = imageIDs;
	this->animSeconds = animSeconds;
	this->shadingType = shadingType;
}

void SkyLandDefinition::init(ImageID imageID, ShadingType shadingType)
{
	TextureUtils::ImageIdGroup imageIDs(imageID, 1);
	constexpr double animSeconds = 0.0;
	this->init(imageIDs, animSeconds, shadingType);
}

int SkyLandDefinition::getImageCount() const
{
	return this->imageIDs.getCount();
}

ImageID SkyLandDefinition::getImageID(int index) const
{
	return this->imageIDs.getID(index);
}

bool SkyLandDefinition::hasAnimation() const
{
	return imageIDs.getCount() > 1;
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
