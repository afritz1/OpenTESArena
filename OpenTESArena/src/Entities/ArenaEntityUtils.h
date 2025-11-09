#ifndef ARENA_ENTITY_UTILS_H
#define ARENA_ENTITY_UTILS_H

#include <cstdint>
#include <string>

#include "../Stats/PrimaryAttribute.h"

class Random;

enum class ArenaCityType;
enum class ArenaInteriorType;
enum class ArmorMaterialType;

struct ExeData;

using ItemMaterialDefinitionID = int;
using SpellDefinitionID = int;

// The loot generation labels these as able to house an item or not.
struct ArenaValidLootSlots
{
	static constexpr int COUNT = 4;

	bool slots[4];

	ArenaValidLootSlots();
};

namespace ArenaEntityUtils
{
	// For monsters
	int getBaseSpeed(int speedAttribute);

	int getCreatureGold(int creatureLevel, uint32_t creatureLootChance, Random &random);
	bool getCreatureHasMagicItem(int creatureLevel, uint32_t creatureLootChance, Random &random);
	bool getCreatureHasNonMagicWeaponOrArmor(uint32_t creatureLootChance, Random &random);
	bool getCreatureHasMagicWeaponOrArmor(int creatureLevel, uint32_t creatureLootChance, Random &random);

	void getCreatureMagicItem(int creatureLevel, const ExeData &exeData, Random &random, int *outItemID, bool *outIsPotion,
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellDefinitionID *outSpellID);
	int pickPotion(Random &random);
	void pickMagicAccessoryOrTrinket(int specifiedItemID, int quality, const ExeData &exeData, Random &random, int *outItemID,
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellDefinitionID *outSpellID);
	void pickSpellCastingItem(int specifiedItemID, int quality, const ExeData &exeData, Random &random, int *outItemID, SpellDefinitionID *outSpellID);
	void pickAttributeEnhancementItem(int specifiedItemID, int quality, const ExeData &exeData, Random &random, int *outItemID, PrimaryAttributeID *outAttributeID);
	void pickArmorClassItem(int specifiedItemID, const ExeData &exeData, Random &random, int *outItemID, ItemMaterialDefinitionID *outMaterialID);

	// Weapon/armor ID is 0 = staff/cuirass, 1 = dagger/gauntlets, etc.
	int pickNonMagicArmor(int armorLevel, int baseMaterial, int specifiedItemID, const ExeData &exeData, Random &random);
	int pickNonMagicWeapon(int weaponLevel, int specifiedItemID, const ExeData &exeData, Random &random);

	void getCreatureNonMagicWeaponOrArmor(int creatureLevel, const ExeData &exeData, Random &random, int *outWeaponOrArmorID,
		bool *outIsArmor, ArmorMaterialType *outArmorMaterialType);
	int getCreatureNonMagicWeaponOrArmorCondition(int maxCondition, const ExeData &exeData, Random &random);
	int getCreatureItemQualityLevel(int creatureLevel);

	int getHumanEnemyGold(int charClassDefID, const ExeData &exeData, Random &random);

	constexpr int LOOT_VALUES_INDEX_HOUSE = 0;
	constexpr int LOOT_VALUES_INDEX_PALACE = 1;
	constexpr int LOOT_VALUES_INDEX_NOBLE = 2;
	constexpr int LOOT_VALUES_INDEX_DUNGEON = 3;
	constexpr int LOOT_VALUES_INDEX_CRYPT = 4;
	constexpr int LOOT_VALUES_INDEX_TOWER = 4;

	int getLootValuesIndex(ArenaInteriorType interiorType);
	ArenaValidLootSlots getPopulatedLootSlots(int lootValuesIndex, const ExeData &exeData, Random &random);
	int getLootGoldAmount(int lootValuesIndex, const ExeData &exeData, Random &random, ArenaCityType cityType, int levelIndex);
	void getLootMagicItem(int lootValuesIndex, ArenaCityType cityType, int levelIndex, const ExeData &exeData, Random &random, int *outItemID, bool *outIsPotion,
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellDefinitionID *outSpellID);
	int getLootItemQualityValue(int lootValuesIndex, Random &random, ArenaCityType cityType, int levelIndex);
	void getLootNonMagicWeaponOrArmor(const ExeData &exeData, Random &random, int *outWeaponOrArmorID, bool *outIsArmor, ArmorMaterialType *outArmorMaterialType);
	int getLootNonMagicWeaponOrArmorCondition(int lootValuesIndex, const ExeData &exeData, Random &random, int itemMaxHealth);

	std::string getArmorNameFromItemID(int itemID, const ExeData &exeData);
	std::string getWeaponNameFromItemID(int itemID, const ExeData &exeData);
}

#endif
