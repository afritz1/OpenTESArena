#ifndef SKY_LAND_DEFINITION_H
#define SKY_LAND_DEFINITION_H

#include "../Assets/TextureAssetReference.h"
#include "../Media/TextureUtils.h"

class SkyLandDefinition
{
public:
	enum class ShadingType
	{
		Ambient, // Affected by ambient sky intensity.
		Bright // Max brightness.
	};
private:
	std::vector<TextureAssetReference> textureAssetRefs;
	double animSeconds;
	ShadingType shadingType;
public:
	// Initializer for an animated land.
	void init(std::vector<TextureAssetReference> &&textureAssetRefs, double animSeconds, ShadingType shadingType);

	// Initializer for a non-animated land.
	void init(TextureAssetReference &&textureAssetRef, ShadingType shadingType);

	int getTextureCount() const;
	const TextureAssetReference &getTextureAssetRef(int index) const;

	bool hasAnimation() const;
	double getAnimationSeconds() const;

	ShadingType getShadingType() const;
};

#endif
