#ifndef SKY_SUN_DEFINITION_H
#define SKY_SUN_DEFINITION_H

#include "../Assets/TextureAsset.h"

struct SkySunDefinition
{
	TextureAsset textureAsset;

	void init(TextureAsset &&textureAsset);
};

#endif
