#ifndef ARENA_ENTITY_UTILS_H
#define ARENA_ENTITY_UTILS_H

#include <cstdint>
#include <string>

class Random;

enum class ArenaCityType;
enum class ArenaInteriorType;

struct ExeData;

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

	constexpr int LOOT_VALUES_INDEX_HOUSE = 0;
	constexpr int LOOT_VALUES_INDEX_PALACE = 1;
	constexpr int LOOT_VALUES_INDEX_NOBLE = 2;
	constexpr int LOOT_VALUES_INDEX_DUNGEON = 3;
	constexpr int LOOT_VALUES_INDEX_CRYPT = 4;
	constexpr int LOOT_VALUES_INDEX_TOWER = 4;
	constexpr int LootSlotCount = 4;

	int getLootValuesIndex(ArenaInteriorType interiorType);
	std::array<bool, LootSlotCount> getPopulatedLootSlots(int lootValuesIndex, const ExeData &exeData, Random &random);
	int getLootGoldAmount(int lootValuesIndex, const ExeData &exeData, Random &random, ArenaCityType cityType, int levelIndex);
	int getLootItemQualityValue(int lootValuesIndex, Random &random, ArenaCityType cityType, int levelIndex);
	void getLootNonMagicWeaponOrArmor(const ExeData &exeData, Random &random, int *outWeaponOrArmorID, bool *outIsArmor);
	int getLootNonMagicWeaponOrArmorCondition(int lootValuesIndex, const ExeData &exeData, Random &random, int itemMaxHealth);

	std::string getArmorNameFromItemID(int itemID, const ExeData &exeData);
	std::string getWeaponNameFromItemID(int itemID, const ExeData &exeData);
}

#endif
