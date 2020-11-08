#ifndef LAND_OBJECT_DEFINITION_H
#define LAND_OBJECT_DEFINITION_H

#include "../Math/MathUtils.h"
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
	Radians angle;
	TextureManager::IdGroup<ImageID> imageIDs;
	double animSeconds;
public:
	// Initializer for an animated land.
	void init(Radians angle, const TextureManager::IdGroup<ImageID> &imageIDs, double animSeconds);

	// Initializer for a non-animated land.
	void init(Radians angle, ImageID imageID);

	Radians getAngle() const;

	int getImageCount() const;
	ImageID getImageID(int index) const;

	bool hasAnimation() const;
	double getAnimationSeconds() const;
};

#endif
