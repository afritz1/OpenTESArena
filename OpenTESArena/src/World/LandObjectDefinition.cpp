#include "LandObjectDefinition.h"

#include "components/debug/Debug.h"

namespace
{
	constexpr double NO_ANIMATION_SECONDS = 0.0;
}

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
	this->init(angle, imageIDs, NO_ANIMATION_SECONDS);
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
	return this->animSeconds >= NO_ANIMATION_SECONDS;
}

double LandObjectDefinition::getAnimationSeconds() const
{
	DebugAssert(this->hasAnimation());
	return this->animSeconds;
}
