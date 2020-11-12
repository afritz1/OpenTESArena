#ifndef SUN_OBJECT_DEFINITION_H
#define SUN_OBJECT_DEFINITION_H

#include "../Media/TextureUtils.h"

class SunObjectDefinition
{
private:
	ImageID imageID;
public:
	void init(ImageID imageID);

	ImageID getImageID() const;
};

#endif
