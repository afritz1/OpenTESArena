#ifndef SKY_INSTANCE_H
#define SKY_INSTANCE_H

#include <vector>

#include "../Math/Vector3.h"
#include "../Media/TextureUtils.h"

// Contains distant sky object instances and their state.

// The renderer should only care about 1) current direction, 2) current texture ID, 3) anchor,
// and 4) shading type. Maybe also rendering order. It has the option of doing visibility culling
// as well.

class SkyDefinition;
class SkyInfoDefinition;

class SkyInstance
{
private:
	struct ObjectInstance
	{
		Double3 baseDirection; // Position in sky before transformation.
		Double3 transformedDirection; // Position in sky usable by other systems (may be updated frequently).
		ImageID imageID; // Current texture of object (may change due to animation).
		double width, height;

		ObjectInstance(const Double3 &baseDirection, ImageID imageID, double width, double height);
	};

	// Animation data for each sky object with an animation.
	struct AnimInstance
	{
		int objectIndex;
		TextureUtils::ImageIdGroup imageIDs; // All image IDs for the animation.
		double targetSeconds, currentSeconds;

		AnimInstance();

		void init(int objectIndex, const TextureUtils::ImageIdGroup &imageIDs, double targetSeconds);
	};

	std::vector<ObjectInstance> objectInsts; // Each sky object instance.
	std::vector<AnimInstance> animInsts; // Data for each sky object with an animation.
	int landStart, landEnd, airStart, airEnd, starStart, starEnd, sunStart, sunEnd, moonStart, moonEnd;
public:
	void init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition);

	// Start (inclusive) and end (exclusive) indices of each sky object type.
	int getLandStartIndex() const;
	int getLandEndIndex() const;
	int getAirStartIndex() const;
	int getAirEndIndex() const;
	int getStarStartIndex() const;
	int getStarEndIndex() const;
	int getSunStartIndex() const;
	int getSunEndIndex() const;
	int getMoonStartIndex() const;
	int getMoonEndIndex() const;

	void getObject(int index, Double3 *outDirection, ImageID *outImageID, double *outWidth,
		double *outHeight) const;

	void update(double dt, double latitude, double daytimePercent);
};

#endif
