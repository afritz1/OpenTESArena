#include <climits>

#include "ArenaMathUtils.h"

ArenaDiceParameters::ArenaDiceParameters()
{
	this->exclusiveMax = 0;
	this->rollCount = 0;
}

void ArenaMathUtils::rotatePoint(int32_t angle, int16_t &x, int16_t &y, Span<const int16_t> cosineTable)
{
	const int16_t cosAngleMultiplier = cosineTable[angle];
	const int16_t sinAngleMultiplier = cosineTable[angle + 128];

	const int16_t doubledX = x * 2;
	const int16_t doubledY = y * 2;

	int16_t negCosAngleMultiplier = -cosAngleMultiplier;
	const bool isOverflow = cosAngleMultiplier == SHRT_MIN;
	if (isOverflow)
	{
		negCosAngleMultiplier--;
	}

	const int32_t imulRes1 = doubledX * sinAngleMultiplier;
	const int32_t imulRes2 = doubledY * negCosAngleMultiplier;
	const int32_t imulRes3 = doubledX * cosAngleMultiplier;
	const int32_t imulRes4 = doubledY * sinAngleMultiplier;

	const int16_t highRes1 = static_cast<uint32_t>(imulRes1) >> 16;
	const int16_t highRes2 = static_cast<uint32_t>(imulRes2) >> 16;
	const int16_t highRes3 = static_cast<uint32_t>(imulRes3) >> 16;
	const int16_t highRes4 = static_cast<uint32_t>(imulRes4) >> 16;

	x = highRes2 + highRes1;
	y = highRes3 + highRes4;
}

ArenaDiceParameters ArenaMathUtils::getDiceParameters(int value)
{
	while (true)
	{
		for (int divisor = 6; divisor > 4; divisor--)
		{
			const bool isValueDivisible = (value % divisor) == 0;
			if (isValueDivisible)
			{
				ArenaDiceParameters diceParameters;
				diceParameters.exclusiveMax = divisor + 1;
				diceParameters.rollCount = value / divisor;
				return diceParameters;
			}
		}

		value++;
	}
}

int ArenaMathUtils::rollDice(int exclusiveMax, int rollCount, ArenaRandom &random)
{
	int sum = 0;

	for (int i = 0; i < rollCount; i++)
	{
		sum += random.next(exclusiveMax);
	}

	return sum;
}

int ArenaMathUtils::rollDice(const ArenaDiceParameters &diceParameters, ArenaRandom &random)
{
	return ArenaMathUtils::rollDice(diceParameters.exclusiveMax, diceParameters.rollCount, random);
}

int ArenaMathUtils::rollBoundedDice(int maxValue, ArenaRandom &random)
{
	if (maxValue <= 8)
	{
		return random.next(maxValue);
	}

	const ArenaDiceParameters diceParameters = ArenaMathUtils::getDiceParameters(maxValue);
	const int result = ArenaMathUtils::rollDice(diceParameters, random);
	if (result > maxValue)
	{
		return ArenaMathUtils::rollBoundedDice(maxValue, random);
	}

	return result;
}
