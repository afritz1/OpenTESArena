#ifndef SKY_LIGHTNING_DEFINITION_H
#define SKY_LIGHTNING_DEFINITION_H

#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

// Similar to animated land but conditionally rendered based on thunderstorm state.

class SkyLightningDefinition
{
private:
	Buffer<TextureAsset> textureAssets;
	double animSeconds;
public:
	void init(Buffer<TextureAsset> &&textureAssets, double animSeconds);

	int getTextureCount() const;
	const TextureAsset &getTextureAsset(int index) const;

	double getAnimationSeconds() const;
};

#endif
