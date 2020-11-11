#include "SkyUtils.h"

int SkyUtils::getOctantIndex(bool posX, bool posY, bool posZ)
{
	// Use lowest 3 bits to represent 0-7.
	const char xBit = posX ? 0 : (1 << 0);
	const char yBit = posY ? 0 : (1 << 1);
	const char zBit = posZ ? 0 : (1 << 2);
	return xBit | yBit | zBit;
}
