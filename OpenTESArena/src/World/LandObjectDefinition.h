#ifndef LAND_OBJECT_DEFINITION_H
#define LAND_OBJECT_DEFINITION_H

#include "../Media/TextureManager.h"

class LandObjectDefinition
{
public:
	enum class ShadingType
	{
		Ambient, // Affected by ambient sky intensity.
		Bright // Max brightness.
	};
private:
	TextureManager::IdGroup<ImageID> imageIDs;
	double animSeconds;
	ShadingType shadingType;
public:
	// Initializer for an animated land.
	void init(const TextureManager::IdGroup<ImageID> &imageIDs, double animSeconds,
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
