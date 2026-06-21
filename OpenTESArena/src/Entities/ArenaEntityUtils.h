#pragma once

#include <cstdint>
#include <string>

#include "../Assets/ArenaTypes.h"
#include "../Spells/SpellDefinition.h"
#include "../Stats/PrimaryAttribute.h"

#include "components/utilities/Span.h"

class ArenaRandom;
class Random;

enum class ArenaBuildingType;
enum class ArenaCityType;
enum class ArenaEnvironmentType;
enum class ArenaInteriorType;

struct ExeData;

using ItemMaterialDefinitionID = int;

// The loot generation labels these as able to house an item or not.
struct ArenaValidLootSlots
{
	static constexpr int COUNT = 4;

	bool slots[4];

	ArenaValidLootSlots();
};

struct ArenaEntitySpawnPoint
{
	// In Arena units (128 = one voxel).
	int x;
	int z;

	uint16_t tileIndex;

	ArenaEntitySpawnPoint();
	ArenaEntitySpawnPoint(int x, int z, uint16_t tileIndex);
};

struct ArenaEnemyEncounter
{
	uint8_t count;
	uint8_t id;
	uint8_t level;

	ArenaEnemyEncounter();
};

namespace ArenaEntityUtils
{
	// Note that creature ID == creature index + 1.
	// The final boss is a special case, essentially hardcoded at the end of the creatures.
	constexpr ArenaCreatureID FinalBossCreatureID = 24;
	constexpr int CreatureCount = FinalBossCreatureID;

	// For monsters
	int getBaseSpeed(int speedAttribute);

	int getCreatureGold(int creatureLevel, uint32_t creatureLootChance, ArenaRandom &random);
	bool getCreatureHasMagicItem(int creatureLevel, uint32_t creatureLootChance, ArenaRandom &random);
	bool getCreatureHasNonMagicWeaponOrArmor(uint32_t creatureLootChance, ArenaRandom &random);
	bool getCreatureHasMagicWeaponOrArmor(int creatureLevel, uint32_t creatureLootChance, ArenaRandom &random);

	void getCreatureMagicItem(int creatureLevel, const ExeData &exeData, ArenaRandom &random, int *outItemID, bool *outIsPotion,
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID);
	int pickPotion(ArenaRandom &random);
	void pickMagicAccessoryOrTrinket(int specifiedItemID, int qualityThreshold, const ExeData &exeData, ArenaRandom &random, int *outItemID,
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID);
	void pickSpellCastingItem(int specifiedItemID, int qualityThreshold, const ExeData &exeData, ArenaRandom &random, int *outItemID, SpellID *outSpellID);
	void pickAttributeEnhancementItem(int specifiedItemID, int qualityThreshold, const ExeData &exeData, ArenaRandom &random, int *outItemID, PrimaryAttributeID *outAttributeID);
	void pickArmorClassItem(int specifiedItemID, const ExeData &exeData, ArenaRandom &random, int *outItemID, ItemMaterialDefinitionID *outMaterialID);

	// Weapon/armor ID is 0 = staff/cuirass, 1 = dagger/gauntlets, etc.
	ArenaArmorTypeID pickNonMagicArmor(int itemQualityThreshold, ArenaArmorMaterialType baseMaterial, ArenaArmorTypeID specifiedItemID, const ExeData &exeData, ArenaRandom &random);
	ArenaWeaponTypeID pickNonMagicWeapon(int weaponQualityThreshold, ArenaWeaponTypeID specifiedItemID, const ExeData &exeData, ArenaRandom &random);

	void getHumanEnemyArmor(int classNumber, int level, const ExeData &exeData, ArenaRandom &random, Span<ArenaArmorTypeID> outArmorIDs, ArenaArmorMaterialType *outArmorMaterialType);
	void getHumanEnemyWeapon(int classNumber, const ExeData &exeData, ArenaRandom &random, ArenaWeaponTypeID *outWeaponID);
	void getHumanEnemyShield(int classNumber, const ExeData &exeData, ArenaRandom &random, ArenaWeaponTypeID weaponID, ArenaArmorTypeID *outShieldID);
	void getCreatureNonMagicWeaponOrArmor(int creatureLevel, const ExeData &exeData, ArenaRandom &random, int *outWeaponOrArmorID,
		bool *outIsArmor, ArenaArmorMaterialType *outArmorMaterialType);
	int getCreatureNonMagicWeaponOrArmorCondition(int maxCondition, const ExeData &exeData, ArenaRandom &random);
	int getCreatureItemQualityThreshold(int creatureLevel);

	int getHumanEnemyGold(int charClassDefID, const ExeData &exeData, ArenaRandom &random);
	int getHumanEnemyExperience(int level, int charClassDefID, const ExeData &exeData);
	bool getHumanEnemyIsFemale(ArenaRandom &random, const ExeData &exeData);
	ArenaRaceType getHumanEnemyRace(ArenaRandom &random);

	constexpr int LOOT_VALUES_INDEX_HOUSE = 0;
	constexpr int LOOT_VALUES_INDEX_PALACE = 1;
	constexpr int LOOT_VALUES_INDEX_NOBLE = 2;
	constexpr int LOOT_VALUES_INDEX_DUNGEON = 3;
	constexpr int LOOT_VALUES_INDEX_CRYPT = 4;
	constexpr int LOOT_VALUES_INDEX_TOWER = 4;

	int getLootValuesIndex(ArenaInteriorType interiorType);
	ArenaValidLootSlots getPopulatedLootSlots(int lootValuesIndex, const ExeData &exeData, ArenaRandom &random);
	int getLootGoldAmount(int lootValuesIndex, const ExeData &exeData, ArenaRandom &random, ArenaCityType cityType, int levelIndex);
	void getLootMagicItem(int lootValuesIndex, ArenaCityType cityType, int levelIndex, const ExeData &exeData, ArenaRandom &random, int *outItemID, bool *outIsPotion,
		ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID);
	int getLootItemQualityThreshold(int lootValuesIndex, ArenaRandom &random, ArenaCityType cityType, int levelIndex);
	void getLootNonMagicWeaponOrArmor(const ExeData &exeData, ArenaRandom &random, int *outWeaponOrArmorID, bool *outIsArmor, ArenaArmorMaterialType *outArmorMaterialType);
	int getLootNonMagicWeaponOrArmorCondition(int lootValuesIndex, const ExeData &exeData, ArenaRandom &random, int itemMaxHealth);

	std::string getArmorNameFromItemID(int itemID, const ExeData &exeData);
	std::string getWeaponNameFromItemID(int itemID, const ExeData &exeData);

	bool doGuardsAppearForViolence(int playerLevel, ArenaRandom &random);
	bool doGuardsAppearForTheft(int thievingSkill, ArenaRandom &random);
	int getGuardType(const ExeData &exeData, ArenaRandom &random);
	int getGuardLevel(ArenaCityType cityType, int tierBonus, const ExeData &exeData, ArenaRandom &random);
	ArenaRaceType getGuardRace(int currentProvinceID);
	int getNumberOfGuardsToSpawn(ArenaRandom &random);
	void getGuardArmor(int guardType, const ExeData &exeData, ArenaRandom &random, Span<ArenaArmorTypeID> outArmorIDs, ArenaArmorMaterialType *outArmorMaterialType);
	ArenaWeaponTypeID getGuardWeapon(const ExeData &exeData, ArenaRandom &random);
	ArenaArmorTypeID getGuardShield(int guardType, const ExeData &exeData, ArenaRandom &random);

	bool isEnemyEncounterAllowedOnMinuteChanged(ArenaEnvironmentType environmentType, bool areCitizensPresent, bool isPlayerCamping, bool isPlayerOnRaisedPlatform);
	bool isEnemyEncounterAllowedOnHourChanged(ArenaEnvironmentType environmentType, bool isPlayerCamping, bool isPlayerOnRaisedPlatform);
	void getEncounterParameters(ArenaEnvironmentType currentEnvironmentType, ArenaBuildingType currentBuildingType, bool playerTrespassing, bool isResting, int terrainType, int gameHour, int dayOfYear, const ExeData &exeData, int *outEncounterChance, int *outEncounterTableIndex);
	int rollWeightedEncounterLevel(int encounterLevel, ArenaRandom &random);
	ArenaEnemyEncounter chooseEncounterEnemy(int encounterTableIndex, int encounterLevel, int playerLevel, bool mainQuestEncounter, ArenaRandom &random, const ExeData &exeData);

	ArenaEntitySpawnPoint findRandomSpawnLocationAroundPlayer(int16_t playerX, int16_t playerZ, /*const uint16_t *map1, const uint16_t *floorMap, uint16_t invalidFloorThreshold, const std::array<uint16_t, 23> &occupiedTiles, uint16_t playerTileIndex,*/ ArenaRandom &random);
	bool isTileOccupied(uint16_t tileIndex, Span<const uint16_t> occupiedTiles);
	bool isSpawnTileValid(uint16_t tileIndex, Span<const uint16_t> map1, Span<const uint16_t> flor, uint16_t invalidFloorThreshold);

	int snapToTileCenter(int16_t coord);
	uint16_t makeTileIndex(int16_t x, int16_t z);

	void paralysisOrDiseaseOnHit(int creatureIndex, int playerRace, int playerClass, bool isPlayerPoisonResistEffectActive, int playerPoisonSavingThrow, ArenaRandom &random, const ExeData &exeData, int *outAppliedDiseaseID, double *outAppliedDiseaseSeconds, double *outAppliedParalysisSeconds);
	void causeDisease(int diseaseID, ArenaRandom &random, const ExeData &exeData, double *outAppliedDiseaseSeconds);
}
