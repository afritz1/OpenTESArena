#include "LandObjectDefinition.h"

#include "components/debug/Debug.h"

void LandObjectDefinition::init(Radians angle, const TextureManager::IdGroup<ImageID> &imageIDs,
	double animSeconds)
{
	this->angle = angle;
	this->imageIDs = imageIDs;
	this->animSeconds = animSeconds;
}

void LandObjectDefinition::init(Radians angle, ImageID imageID)
{
	TextureManager::IdGroup<ImageID> imageIDs(imageID, 1);
	constexpr double animSeconds = 0.0;
	this->init(angle, imageIDs, animSeconds);
}

Radians LandObjectDefinition::getAngle() const
{
	return this->angle;
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
