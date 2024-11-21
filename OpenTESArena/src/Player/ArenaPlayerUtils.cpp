#include <cmath>

#include "ArenaPlayerUtils.h"

int ArenaPlayerUtils::getBaseSpeed(int speedAttribute, int encumbranceMod)
{
	return ((((speedAttribute * 20) / 256) * (256 - encumbranceMod)) / 256) + 20;
}

int ArenaPlayerUtils::getMoveSpeed(int baseSpeed)
{
	return baseSpeed;
}

int ArenaPlayerUtils::getTurnSpeed(int baseSpeed)
{
	return (baseSpeed / 2) + 13;
}

int ArenaPlayerUtils::getChasmFallSpeed(int frame)
{
	return static_cast<int>(std::pow(2, 2 + (frame / 2)));
}

int ArenaPlayerUtils::getJumpUnitsPerFrame(int frame)
{
	return 10 - (2 * frame);
}
