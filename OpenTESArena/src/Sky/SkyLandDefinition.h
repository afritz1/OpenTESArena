#ifndef SKY_LAND_DEFINITION_H
#define SKY_LAND_DEFINITION_H

#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"

#include "components/utilities/Buffer.h"

enum class SkyLandShadingType
{
	Ambient, // Affected by ambient sky intensity.
	Bright // Max brightness.
};

struct SkyLandDefinition
{
	Buffer<TextureAsset> textureAssets;
	double animSeconds;
	bool hasAnimation;
	SkyLandShadingType shadingType;

	// Animated land.
	void init(Buffer<TextureAsset> &&textureAssets, double animSeconds, SkyLandShadingType shadingType);

	// Non-animated land.
	void init(TextureAsset &&textureAsset, SkyLandShadingType shadingType);
};

#endif
