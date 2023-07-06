#ifndef SKY_AIR_DEFINITION_H
#define SKY_AIR_DEFINITION_H

#include "../Assets/TextureAsset.h"

struct SkyAirDefinition
{
	TextureAsset textureAsset;

	void init(TextureAsset &&textureAsset);
};

#endif
