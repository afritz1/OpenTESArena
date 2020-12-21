#ifndef SKY_MOON_DEFINITION_H
#define SKY_MOON_DEFINITION_H

#include "../Media/TextureManager.h"

class SkyMoonDefinition
{
private:
	TextureUtils::ImageIdGroup imageIDs;
public:
	void init(const TextureUtils::ImageIdGroup &imageIDs);

	int getImageIdCount() const;
	ImageID getImageID(int index) const;
};

#endif
