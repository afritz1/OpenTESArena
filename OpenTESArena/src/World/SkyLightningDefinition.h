#ifndef SKY_LIGHTNING_DEFINITION_H
#define SKY_LIGHTNING_DEFINITION_H

#include "../Assets/TextureAssetReference.h"

#include "components/utilities/Buffer.h"

// Similar to animated land but conditionally rendered based on thunderstorm state.

class SkyLightningDefinition
{
private:
	Buffer<TextureAssetReference> textureAssetRefs;
	double animSeconds;
public:
	void init(Buffer<TextureAssetReference> &&textureAssetRefs, double animSeconds);

	int getTextureCount() const;
	const TextureAssetReference &getTextureAssetRef(int index) const;

	double getAnimationSeconds() const;
};

#endif
