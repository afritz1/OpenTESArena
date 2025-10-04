#include "ArenaEntityUtils.h"
#include "../Math/Random.h"

int ArenaEntityUtils::getBaseSpeed(int speedAttribute)
{
	return ((((speedAttribute * 20) / 256) * 256) / 256) + 20;
}

int ArenaEntityUtils::getCreatureGold(int creatureLevel, unsigned int creatureLootChance, Random &random)
{
	if (1 + random.next(100) <= (creatureLootChance & 0xFF) && random.next(101) >= (creatureLootChance & 0xFF))
		return (1 + random.next(10)) * (creatureLevel + 1);
	else
		return 0;
}

bool ArenaEntityUtils::getCreatureHasMagicItem(int creatureLevel, unsigned int creatureLootChance, Random &random)
{
	if (creatureLevel > 2 && (1 + random.next(100) <= ((creatureLootChance & 0xFF00) >> 8)))
		return true;
	else
		return false;
}

bool ArenaEntityUtils::getCreatureHasNonMagicWeaponOrArmor(unsigned int creatureLootChance, Random& random)
{
	if (1 + random.next(100) <= ((creatureLootChance & 0xFF0000) >> 16))
		return true;
	else
		return false;
}

bool ArenaEntityUtils::getCreatureHasMagicWeaponOrArmor(int creatureLevel, unsigned int creatureLootChance, Random& random)
{
	if (creatureLevel > 6 && (1 + random.next(100) <= ((creatureLootChance & 0xFF000000) >> 24)))
		return true;
	else
		return false;
}

int ArenaEntityUtils::getCreatureItemQualityLevel(int creatureLevel)
{
	return creatureLevel + 1;
}
