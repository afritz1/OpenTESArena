#include "ArenaEntityUtils.h"
#include "../Math/Random.h"

int ArenaEntityUtils::getBaseSpeed(int speedAttribute)
{
	return ((((speedAttribute * 20) / 256) * 256) / 256) + 20;
}

int ArenaEntityUtils::getCreatureCorpseGold(int creatureLevel, Random &random)
{
	return (1 + random.next(11)) * (creatureLevel + 1);
}

int ArenaEntityUtils::getCreatureItemQualityLevel(int creatureLevel)
{
	return creatureLevel + 1;
}
