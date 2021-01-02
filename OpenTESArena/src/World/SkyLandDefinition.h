#ifndef SKY_LAND_DEFINITION_H
#define SKY_LAND_DEFINITION_H

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
	TextureBuilderIdGroup textureBuilderIDs;
	double animSeconds;
	ShadingType shadingType;
public:
	// Initializer for an animated land.
	void init(const TextureBuilderIdGroup &textureBuilderIDs, double animSeconds, ShadingType shadingType);

	// Initializer for a non-animated land.
	void init(TextureBuilderID textureBuilderID, ShadingType shadingType);

	int getTextureCount() const;
	TextureBuilderID getTextureBuilderID(int index) const;

	bool hasAnimation() const;
	double getAnimationSeconds() const;

	ShadingType getShadingType() const;
};

#endif
