#ifndef MOON_OBJECT_DEFINITION_H
#define MOON_OBJECT_DEFINITION_H

#include "../Media/TextureManager.h"

class MoonObjectDefinition
{
private:
	TextureUtils::ImageIdGroup imageIDs;
public:
	void init(const TextureUtils::ImageIdGroup &imageIDs);

	int getImageIdCount() const;
	ImageID getImageID(int index) const;
};

#endif
