#include "ArenaEntityUtils.h"
#include "../Math/Random.h"

int ArenaEntityUtils::getBaseSpeed(int speedAttribute)
{
	return ((((speedAttribute * 20) / 256) * 256) / 256) + 20;
}

int ArenaEntityUtils::getCreatureGold(int creatureLevel, int creatureLootChance, Random &random)
{
	if (1 + random.next(100) <= (creatureLootChance & 0xFF) && random.next(101) >= (creatureLootChance & 0xFF))
	{
		return (1 + random.next(10)) * (creatureLevel + 1);
	}
	else
		return 0;
}

int ArenaEntityUtils::getCreatureItemQualityLevel(int creatureLevel)
{
	return creatureLevel + 1;
}
