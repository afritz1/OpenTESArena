#ifndef SKY_MOON_DEFINITION_H
#define SKY_MOON_DEFINITION_H

#include "../Assets/TextureAssetReference.h"
#include "../Media/TextureUtils.h"

#include "components/utilities/Buffer.h"

class SkyMoonDefinition
{
private:
	// One texture per phase.
	Buffer<TextureAssetReference> textureAssetRefs;
public:
	void init(Buffer<TextureAssetReference> &&textureAssetRefs);

	int getTextureCount() const;
	const TextureAssetReference &getTextureAssetRef(int index) const;
};

#endif
