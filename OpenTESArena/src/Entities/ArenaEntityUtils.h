#ifndef ARENA_ENTITY_UTILS_H
#define ARENA_ENTITY_UTILS_H

class Random;

namespace ArenaEntityUtils
{
	// For monsters
	int getBaseSpeed(int speedAttribute);

	int getCreatureGold(int creatureLevel, int creatureLootChance, Random &random);
	int getCreatureItemQualityLevel(int creatureLevel);
}

#endif
