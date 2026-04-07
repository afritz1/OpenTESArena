#pragma once

#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

// Similar to animated land but conditionally rendered based on thunderstorm state.
struct SkyLightningDefinition
{
	Buffer<TextureAsset> textureAssets;
	double animSeconds;

	void init(Buffer<TextureAsset> &&textureAssets, double animSeconds);
};
