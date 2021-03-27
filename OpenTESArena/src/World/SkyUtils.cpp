#include <cmath>

#include "ArenaSkyUtils.h"
#include "SkyUtils.h"
#include "../Math/Constants.h"

#include "components/debug/Debug.h"

int SkyUtils::getOctantIndex(bool posX, bool posY, bool posZ)
{
	// Use lowest 3 bits to represent 0-7.
	const char xBit = posX ? 0 : (1 << 0);
	const char yBit = posY ? 0 : (1 << 1);
	const char zBit = posZ ? 0 : (1 << 2);
	return xBit | yBit | zBit;
}

VoxelDouble3 SkyUtils::getSkyObjectDirection(Radians angleX, Radians angleY)
{
	return VoxelDouble3(
		-std::sin(angleX),
		std::sin(angleY),
		-std::cos(angleX)).normalized();
}

void SkyUtils::getSkyObjectDimensions(int imageWidth, int imageHeight, double *outWidth, double *outHeight)
{
	*outWidth = static_cast<double>(imageWidth) / ArenaSkyUtils::IDENTITY_DIM;
	*outHeight = static_cast<double>(imageHeight) / ArenaSkyUtils::IDENTITY_DIM;
}

int SkyUtils::getStarCountFromDensity(int starDensity)
{
	if (starDensity == 0)
	{
		// Classic.
		return 40;
	}
	else if (starDensity == 1)
	{
		// Moderate.
		return 1000;
	}
	else if (starDensity == 2)
	{
		// High.
		return 8000;
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(starDensity));
	}
}
