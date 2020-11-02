#ifndef SKY_OBJECT_DEFINITION_H
#define SKY_OBJECT_DEFINITION_H

#include "../Math/MathUtils.h"
#include "../Math/Vector2.h"
#include "../Media/TextureManager.h"

class SkyObjectDefinition
{
public:
	enum class Type
	{
		Land,
		Air,
		Sun,
		Moon,
		Star
	};

	enum class ShadingType
	{
		Ambient, // Affected by ambient sky intensity.
		Bright, // Max brightness.
		Moon, // Special shadowed regions use sky color.
		Star // Blends depending on sky color/brightness.
	};
private:
	TextureManager::IdGroup<ImageID> imageIDs;
	double animationSeconds;
	ShadingType shadingType;
	Type type;

	// Origin inside of the sky object for rendering offset, with (0, 0) at the top left corner.
	Double2 anchor;

	double width, height;

	void init(Type type, double width, double height, const Double2 &anchor, ShadingType shadingType,
		double animationSeconds, const TextureManager::IdGroup<ImageID> &imageIDs);
public:
	SkyObjectDefinition();

	void initLand(double width, double height); // @todo: filename + TextureManager?
	void initAir(double width, double height);
	void initSun(double width, double height);
	void initMoon(double width, double height);
	void initStar(double width, double height);

	int getImageIdCount() const;
	ImageID getImageID(int index) const;

	bool hasAnimation() const;
	double getAnimationSeconds() const;

	ShadingType getShadingType() const;
	Type getType() const;

	double getWidth() const;
	double getHeight() const;
};

#endif
