#ifndef ARENA_ENTITY_UTILS_H
#define ARENA_ENTITY_UTILS_H

#include <cstdint>

struct ExeData;

class Random;

namespace ArenaEntityUtils
{
	// For monsters
	int getBaseSpeed(int speedAttribute);

	int getCreatureGold(int creatureLevel, uint32_t creatureLootChance, Random &random);
	bool getCreatureHasMagicItem(int creatureLevel, uint32_t creatureLootChance, Random &random);
	bool getCreatureHasNonMagicWeaponOrArmor(uint32_t creatureLootChance, Random &random);
	int getCreatureNonMagicWeaponOrArmor(int creatureLevel, const ExeData& exeData, Random& random);
	int getCreatureNonMagicWeaponOrArmorCondition(int maxCondition, const ExeData& exeData, Random& random);
	bool getCreatureHasMagicWeaponOrArmor(int creatureLevel, uint32_t creatureLootChance, Random &random);
	int getCreatureItemQualityLevel(int creatureLevel);
	int pickNonMagicArmor(int armorLevel, int baseMaterial, int specifiedSlotID, const ExeData& exeData, Random& random);
	int pickNonMagicWeapon(int armorLevel, int specifiedSlotID, const ExeData& exeData, Random& random);

	int getHumanEnemyGold(int charClassDefID, const ExeData &exeData, Random &random);
}

#endif
