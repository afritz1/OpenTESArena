#include "SkyMoonDefinition.h"

void SkyMoonDefinition::init(const TextureUtils::ImageIdGroup &imageIDs)
{
	this->imageIDs = imageIDs;
}

int SkyMoonDefinition::getImageIdCount() const
{
	return this->imageIDs.getCount();
}

ImageID SkyMoonDefinition::getImageID(int index) const
{
	return this->imageIDs.getID(index);
}
