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

class SkyLandDefinition
{
private:
	Buffer<TextureAsset> textureAssets;
	double animSeconds;
	SkyLandShadingType shadingType;
public:
	// Initializer for an animated land.
	void init(Buffer<TextureAsset> &&textureAssets, double animSeconds, SkyLandShadingType shadingType);

	// Initializer for a non-animated land.
	void init(TextureAsset &&textureAsset, SkyLandShadingType shadingType);

	int getTextureCount() const;
	const TextureAsset &getTextureAsset(int index) const;

	bool hasAnimation() const;
	double getAnimationSeconds() const;

	SkyLandShadingType getShadingType() const;
};

#endif
