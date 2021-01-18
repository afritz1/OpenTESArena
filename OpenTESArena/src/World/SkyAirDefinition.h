#ifndef SKY_AIR_DEFINITION_H
#define SKY_AIR_DEFINITION_H

#include "../Assets/TextureAssetReference.h"

class SkyAirDefinition
{
private:
	TextureAssetReference textureAssetRef;
public:
	void init(TextureAssetReference &&textureAssetRef);

	const TextureAssetReference &getTextureAssetRef() const;
};

#endif
