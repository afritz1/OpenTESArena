#pragma once

#include <cstdint>

#include "components/utilities/Span.h"
#include "Random.h"

namespace ArenaMathUtils
{
	void rotatePoint(int32_t angle, int16_t &x, int16_t &y, Span<const int16_t> cosineTable);

	struct DiceParameters
	{
		int exclusiveMax;
		int rollCount;
	};

	DiceParameters getDiceParameters(int value);
	int rollDice(int exclusiveMax, int rollCount, ArenaRandom &random);
	int rollBoundedDice(int maxValue, ArenaRandom &random);
}
