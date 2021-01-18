#ifndef SKY_MOON_DEFINITION_H
#define SKY_MOON_DEFINITION_H

#include "../Assets/TextureAssetReference.h"
#include "../Media/TextureUtils.h"

class SkyMoonDefinition
{
private:
	// One texture per phase.
	std::vector<TextureAssetReference> textureAssetRefs;
public:
	void init(std::vector<TextureAssetReference> &&textureAssetRefs);

	int getTextureCount() const;
	const TextureAssetReference &getTextureAssetRef(int index) const;
};

#endif
