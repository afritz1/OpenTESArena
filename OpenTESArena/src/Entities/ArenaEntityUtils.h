#pragma once

#include <cstdint>
#include <string>

#include "../Spells/SpellDefinition.h"
#include "../Stats/PrimaryAttribute.h"

#include "components/utilities/Span.h"

class Random;
class ArenaRandom;

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

struct SpawnPoint
{
	int x;
	int z;
	uint16_t tileIndex;
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

	bool doGuardsAppearForViolence(int playerLevel, ArenaRandom &random);
	bool doGuardsAppearForTheft(int thievingSkill, ArenaRandom &random);
	int getGuardType(const ExeData &exeData, ArenaRandom &random);
	int getGuardLevel(ArenaCityType cityTypeID, int levelBonus, const ExeData &exeData, ArenaRandom &random);
	int getNumberOfGuards(ArenaRandom &random);

	SpawnPoint findRandomSpawnLocationAroundPlayer(int16_t playerX, int16_t playerZ, /*const uint16_t *map1, const uint16_t *floorMap, uint16_t invalidFloorThreshold, const std::array<uint16_t, 23> &occupiedTiles, uint16_t playerTileIndex,*/ ArenaRandom &random);
	bool isTileOccupied(uint16_t tileIndex, const std::array<uint16_t, 23> &occupiedTiles);
	bool isSpawnTileValid(uint16_t tileIndex, const uint16_t *map1, const uint16_t *floorMap, uint16_t invalidFloorThreshold);

	static int snapToTileCenter(int16_t coord);
	static uint16_t makeTileIndex(int16_t x, int16_t z);
}
