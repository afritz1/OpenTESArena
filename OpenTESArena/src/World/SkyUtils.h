#ifndef SKY_UTILS_H
#define SKY_UTILS_H

#include "VoxelUtils.h"
#include "../Math/MathUtils.h"

namespace SkyUtils
{
	// Gets the 0-7 index of a sky octant depending on XYZ coordinate signs.
	int getOctantIndex(bool posX, bool posY, bool posZ);

	// Converts X and Y sky angles to a direction, where x=[0, 2pi) and y=[-pi/2, pi/2].
	VoxelDouble3 getSkyObjectDirection(Radians angleX, Radians angleY);

	// Converts sky object texture dimensions to shape dimensions.
	void getSkyObjectDimensions(int imageWidth, int imageHeight, double *outWidth, double *outHeight);

	// Gets the number of stars to generate based on the given star density (new to this engine).
	int getStarCountFromDensity(int starDensity);
}

#endif
