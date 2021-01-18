#ifndef SKY_SUN_DEFINITION_H
#define SKY_SUN_DEFINITION_H

#include "../Assets/TextureAssetReference.h"

class SkySunDefinition
{
private:
	TextureAssetReference textureAssetRef;
public:
	void init(TextureAssetReference &&textureAssetRef);

	const TextureAssetReference &getTextureAssetRef() const;
};

#endif
