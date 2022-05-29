#ifndef SKY_LAND_DEFINITION_H
#define SKY_LAND_DEFINITION_H

#include "../Assets/TextureAsset.h"
#include "../Media/TextureUtils.h"

#include "components/utilities/Buffer.h"

class SkyLandDefinition
{
public:
	enum class ShadingType
	{
		Ambient, // Affected by ambient sky intensity.
		Bright // Max brightness.
	};
private:
	Buffer<TextureAsset> textureAssets;
	double animSeconds;
	ShadingType shadingType;
public:
	// Initializer for an animated land.
	void init(Buffer<TextureAsset> &&textureAssets, double animSeconds, ShadingType shadingType);

	// Initializer for a non-animated land.
	void init(TextureAsset &&textureAsset, ShadingType shadingType);

	int getTextureCount() const;
	const TextureAsset &getTextureAsset(int index) const;

	bool hasAnimation() const;
	double getAnimationSeconds() const;

	ShadingType getShadingType() const;
};

#endif
