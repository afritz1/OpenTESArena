#pragma once

#include <cstdint>

#include "Random.h"

#include "components/utilities/Span.h"

struct ArenaDiceParameters
{
	int exclusiveMax;
	int rollCount;

	ArenaDiceParameters();
};

namespace ArenaMathUtils
{
	void rotatePoint(int32_t angle, int16_t &x, int16_t &y, Span<const int16_t> cosineTable);

	ArenaDiceParameters getDiceParameters(int value);
	int rollDice(int exclusiveMax, int rollCount, ArenaRandom &random);
	int rollDice(const ArenaDiceParameters &diceParameters, ArenaRandom &random);
	int rollBoundedDice(int maxValue, ArenaRandom &random);
}
