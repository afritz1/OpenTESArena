#ifndef SKY_SUN_DEFINITION_H
#define SKY_SUN_DEFINITION_H

#include "../Assets/TextureAsset.h"

class SkySunDefinition
{
private:
	TextureAsset textureAsset;
public:
	void init(TextureAsset &&textureAsset);

	const TextureAsset &getTextureAsset() const;
};

#endif
