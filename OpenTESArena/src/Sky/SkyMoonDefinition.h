#ifndef SKY_MOON_DEFINITION_H
#define SKY_MOON_DEFINITION_H

#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

struct SkyMoonDefinition
{
	// One texture per phase.
	Buffer<TextureAsset> textureAssets;

	void init(Buffer<TextureAsset> &&textureAssets);
};

#endif
