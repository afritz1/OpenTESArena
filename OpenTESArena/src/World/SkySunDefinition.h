#ifndef SKY_SUN_DEFINITION_H
#define SKY_SUN_DEFINITION_H

#include "../Media/TextureUtils.h"

class SkySunDefinition
{
private:
	ImageID imageID;
public:
	void init(ImageID imageID);

	ImageID getImageID() const;
};

#endif
