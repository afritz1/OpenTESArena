#ifndef SKY_AIR_DEFINITION_H
#define SKY_AIR_DEFINITION_H

#include "../Media/TextureUtils.h"

class SkyAirDefinition
{
private:
	TextureBuilderID textureBuilderID;
public:
	void init(TextureBuilderID textureBuilderID);

	TextureBuilderID getTextureBuilderID() const;
};

#endif
