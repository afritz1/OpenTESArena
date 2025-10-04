#ifndef ARENA_ENTITY_UTILS_H
#define ARENA_ENTITY_UTILS_H

class Random;

namespace ArenaEntityUtils
{
	// For monsters
	int getBaseSpeed(int speedAttribute);

	int getCreatureGold(int creatureLevel, unsigned int creatureLootChance, Random &random);
	bool getCreatureHasMagicItem(int creatureLevel, unsigned int creatureLootChance, Random& random);
	bool getCreatureHasNonMagicWeaponOrArmor(unsigned int creatureLootChance, Random& random);
	bool getCreatureHasMagicWeaponOrArmor(int creatureLevel, unsigned int creatureLootChance, Random& random);
	int getCreatureItemQualityLevel(int creatureLevel);
}

#endif
