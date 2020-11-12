#ifndef AIR_OBJECT_DEFINITION_H
#define AIR_OBJECT_DEFINITION_H

#include "../Media/TextureUtils.h"

class AirObjectDefinition
{
private:
	ImageID imageID;
public:
	void init(ImageID imageID);

	ImageID getImageID() const;
};

#endif
