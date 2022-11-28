#ifndef SKY_MOON_DEFINITION_H
#define SKY_MOON_DEFINITION_H

#include "../Assets/TextureAsset.h"
#include "../Media/TextureUtils.h"

#include "components/utilities/Buffer.h"

class SkyMoonDefinition
{
private:
	// One texture per phase.
	Buffer<TextureAsset> textureAssets;
public:
	void init(Buffer<TextureAsset> &&textureAssets);

	int getTextureCount() const;
	const TextureAsset &getTextureAsset(int index) const;
};

#endif
