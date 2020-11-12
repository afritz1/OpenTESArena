#include "MoonObjectDefinition.h"

void MoonObjectDefinition::init(const TextureManager::IdGroup<ImageID> &imageIDs)
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
