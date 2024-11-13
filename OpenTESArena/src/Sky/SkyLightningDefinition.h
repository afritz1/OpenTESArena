#ifndef SKY_LIGHTNING_DEFINITION_H
#define SKY_LIGHTNING_DEFINITION_H

#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

// Similar to animated land but conditionally rendered based on thunderstorm state.
struct SkyLightningDefinition
{
	Buffer<TextureAsset> textureAssets;
	double animSeconds;

	void init(Buffer<TextureAsset> &&textureAssets, double animSeconds);
};

#endif
