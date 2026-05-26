#pragma once

#include <cstdint>
#include <string>

#include "../Spells/SpellDefinition.h"
#include "../Stats/PrimaryAttribute.h"

#include "components/utilities/Span.h"

class Random;

enum class ArenaCityType;
enum class ArenaInteriorType;
enum class ArmorMaterialType;

struct ExeData;

using ItemMaterialDefinitionID = int;

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
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID);
	int pickPotion(Random &random);
	void pickMagicAccessoryOrTrinket(int specifiedItemID, int qualityThreshold, const ExeData &exeData, Random &random, int *outItemID,
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID);
	void pickSpellCastingItem(int specifiedItemID, int qualityThreshold, const ExeData &exeData, Random &random, int *outItemID, SpellID *outSpellID);
	void pickAttributeEnhancementItem(int specifiedItemID, int qualityThreshold, const ExeData &exeData, Random &random, int *outItemID, PrimaryAttributeID *outAttributeID);
	void pickArmorClassItem(int specifiedItemID, const ExeData &exeData, Random &random, int *outItemID, ItemMaterialDefinitionID *outMaterialID);

	// Weapon/armor ID is 0 = staff/cuirass, 1 = dagger/gauntlets, etc.
	int pickNonMagicArmor(int itemQualityThreshold, int baseMaterial, int specifiedItemID, const ExeData &exeData, Random &random);
	int pickNonMagicWeapon(int weaponQualityThreshold, int specifiedItemID, const ExeData &exeData, Random &random);

	void getHumanEnemyArmor(int classNumber, int level, const ExeData &exeData, Random &random, Span<int> outArmorIDs, ArmorMaterialType *outArmorMaterialType);
	void getHumanEnemyWeapon(int classNumber, const ExeData &exeData, Random &random, int *outWeaponID);
	void getHumanEnemyShield(int classNumber, const ExeData &exeData, Random &random, int weaponID, int *outShieldID);
	void getCreatureNonMagicWeaponOrArmor(int creatureLevel, const ExeData &exeData, Random &random, int *outWeaponOrArmorID,
		bool *outIsArmor, ArmorMaterialType *outArmorMaterialType);
	int getCreatureNonMagicWeaponOrArmorCondition(int maxCondition, const ExeData &exeData, Random &random);
	int getCreatureItemQualityThreshold(int creatureLevel);

	int getHumanEnemyGold(int charClassDefID, const ExeData &exeData, Random &random);
	int getHumanEnemyExperience(int level, int charClassDefID, const ExeData &exeData);

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
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID);
	int getLootItemQualityThreshold(int lootValuesIndex, Random &random, ArenaCityType cityType, int levelIndex);
	void getLootNonMagicWeaponOrArmor(const ExeData &exeData, Random &random, int *outWeaponOrArmorID, bool *outIsArmor, ArmorMaterialType *outArmorMaterialType);
	int getLootNonMagicWeaponOrArmorCondition(int lootValuesIndex, const ExeData &exeData, Random &random, int itemMaxHealth);

	std::string getArmorNameFromItemID(int itemID, const ExeData &exeData);
	std::string getWeaponNameFromItemID(int itemID, const ExeData &exeData);
}
