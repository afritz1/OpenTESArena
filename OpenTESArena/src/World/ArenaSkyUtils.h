#ifndef ARENA_SKY_UTILS_H
#define ARENA_SKY_UTILS_H

#include "../Math/Constants.h"
#include "../Math/MathUtils.h"

enum class ClimateType;

namespace ArenaSkyUtils
{
	// Original game values.
	constexpr int UNIQUE_ANGLES = 512;
	constexpr double IDENTITY_DIM = 320.0;
	constexpr Radians IDENTITY_ANGLE = 90.0 * Constants::DegToRad;
	constexpr double ANIMATED_LAND_SECONDS_PER_FRAME = 1.0 / 18.0;

	// Sun/moon latitudes, divide by 100.0 for modern latitude.
	constexpr double SUN_BONUS_LATITUDE = 13.0;
	constexpr double MOON_1_BONUS_LATITUDE = 15.0;
	constexpr double MOON_2_BONUS_LATITUDE = 30.0;

	// Helper struct for original game's distant land.
	struct LandTraits
	{
		int filenameIndex; // Index into ExeData mountain filenames.
		int position;
		int variation;
		int maxDigits; // Max number of digits in the filename for the variation.

		LandTraits(int filenameIndex, int position, int variation, int maxDigits);
	};

	// Gets distant land traits for a sky by climate.
	const LandTraits &getLandTraits(ClimateType climateType);

	// Converts an Arena angle to an actual angle in radians.
	Radians arenaAngleToRadians(int arenaAngle);
}

#endif
