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
	bool getCreatureHasMagicWeaponOrArmor(int creatureLevel, uint32_t creatureLootChance, Random &random);

	// Weapon/armor ID is 0 = staff/cuirass, 1 = dagger/gauntlets, etc.
	int pickNonMagicArmor(int armorLevel, int baseMaterial, int specifiedItemID, const ExeData &exeData, Random &random);
	int pickNonMagicWeapon(int weaponLevel, int specifiedItemID, const ExeData &exeData, Random &random);

	void getCreatureNonMagicWeaponOrArmor(int creatureLevel, const ExeData &exeData, Random &random, int *outWeaponOrArmorID, bool *outIsArmor);
	int getCreatureNonMagicWeaponOrArmorCondition(int maxCondition, const ExeData &exeData, Random &random);
	int getCreatureItemQualityLevel(int creatureLevel);

	int getHumanEnemyGold(int charClassDefID, const ExeData &exeData, Random &random);
}

#endif
