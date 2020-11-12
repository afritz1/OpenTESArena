#ifndef MOON_OBJECT_DEFINITION_H
#define MOON_OBJECT_DEFINITION_H

#include "../Media/TextureManager.h"

class MoonObjectDefinition
{
private:
	TextureManager::IdGroup<ImageID> imageIDs;
public:
	void init(const TextureManager::IdGroup<ImageID> &imageIDs);

	int getImageIdCount() const;
	ImageID getImageID(int index) const;
};

#endif
