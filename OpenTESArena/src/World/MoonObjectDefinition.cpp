#include "MoonObjectDefinition.h"

void MoonObjectDefinition::init(const TextureUtils::ImageIdGroup &imageIDs)
{
	this->imageIDs = imageIDs;
}

int MoonObjectDefinition::getImageIdCount() const
{
	return this->imageIDs.getCount();
}

ImageID MoonObjectDefinition::getImageID(int index) const
{
	return this->imageIDs.getID(index);
}
