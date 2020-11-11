#ifndef SKY_UTILS_H
#define SKY_UTILS_H

namespace SkyUtils
{
	// Gets the 0-7 index of a sky octant depending on XYZ coordinate signs.
	int getOctantIndex(bool posX, bool posY, bool posZ);
}

#endif
