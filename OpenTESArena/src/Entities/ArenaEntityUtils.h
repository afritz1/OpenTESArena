#ifndef ARENA_ENTITY_UTILS_H
#define ARENA_ENTITY_UTILS_H

#include <cstdint>

class Random;

namespace ArenaEntityUtils
{
	// For monsters
	int getBaseSpeed(int speedAttribute);

	int getCreatureGold(int creatureLevel, uint32_t creatureLootChance, Random &random);
	bool getCreatureHasMagicItem(int creatureLevel, uint32_t creatureLootChance, Random &random);
	bool getCreatureHasNonMagicWeaponOrArmor(uint32_t creatureLootChance, Random &random);
	bool getCreatureHasMagicWeaponOrArmor(int creatureLevel, uint32_t creatureLootChance, Random &random);
	int getCreatureItemQualityLevel(int creatureLevel);
}

#endif
