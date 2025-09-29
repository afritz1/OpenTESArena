#include <climits>

#include "ArenaMathUtils.h"

void ArenaMathUtils::rotatePoint(int32_t angle, int16_t &x, int16_t &y, Span<const int16_t> cosineTable)
{
	const int16_t cosAngleMultiplier = cosineTable[angle];
	const int16_t sinAngleMultiplier = cosineTable[angle + 128];

	const int16_t doubledX = x * 2;
	const int16_t doubledY = y * 2;

	int16_t negCosAngleMultipler = -cosAngleMultiplier;
	const bool isOverflow = cosAngleMultiplier == SHRT_MIN;
	if (isOverflow)
	{
		negCosAngleMultipler--;
	}

	const int32_t imulRes1 = doubledX * sinAngleMultiplier;
	const int32_t imulRes2 = doubledY * negCosAngleMultipler;
	const int32_t imulRes3 = doubledX * cosAngleMultiplier;
	const int32_t imulRes4 = doubledY * sinAngleMultiplier;

	const int16_t highRes1 = static_cast<uint32_t>(imulRes1) >> 16;
	const int16_t highRes2 = static_cast<uint32_t>(imulRes2) >> 16;
	const int16_t highRes3 = static_cast<uint32_t>(imulRes3) >> 16;
	const int16_t highRes4 = static_cast<uint32_t>(imulRes4) >> 16;

	x = highRes2 + highRes1;
	y = highRes3 + highRes4;
}
