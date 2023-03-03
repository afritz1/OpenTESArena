#ifndef SKY_AIR_DEFINITION_H
#define SKY_AIR_DEFINITION_H

#include "../Assets/TextureAsset.h"

class SkyAirDefinition
{
private:
	TextureAsset textureAsset;
public:
	void init(TextureAsset &&textureAsset);

	const TextureAsset &getTextureAsset() const;
};

#endif
