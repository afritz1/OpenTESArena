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
public:
	// Initializer for an animated land.
	void init(const TextureManager::IdGroup<ImageID> &imageIDs, double animSeconds);

	// Initializer for a non-animated land.
	void init(ImageID imageID);

	int getImageCount() const;
	ImageID getImageID(int index) const;

	bool hasAnimation() const;
	double getAnimationSeconds() const;
};

#endif
