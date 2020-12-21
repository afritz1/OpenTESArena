#ifndef SKY_LAND_DEFINITION_H
#define SKY_LAND_DEFINITION_H

#include "../Media/TextureManager.h"

class SkyLandDefinition
{
public:
	enum class ShadingType
	{
		Ambient, // Affected by ambient sky intensity.
		Bright // Max brightness.
	};
private:
	TextureUtils::ImageIdGroup imageIDs;
	double animSeconds;
	ShadingType shadingType;
public:
	// Initializer for an animated land.
	void init(const TextureUtils::ImageIdGroup &imageIDs, double animSeconds,
		ShadingType shadingType);

	// Initializer for a non-animated land.
	void init(ImageID imageID, ShadingType shadingType);

	int getImageCount() const;
	ImageID getImageID(int index) const;

	bool hasAnimation() const;
	double getAnimationSeconds() const;

	ShadingType getShadingType() const;
};

#endif
