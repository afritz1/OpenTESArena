#ifndef SKY_AIR_DEFINITION_H
#define SKY_AIR_DEFINITION_H

#include "../Media/TextureUtils.h"

class SkyAirDefinition
{
private:
	ImageID imageID;
public:
	void init(ImageID imageID);

	ImageID getImageID() const;
};

#endif
