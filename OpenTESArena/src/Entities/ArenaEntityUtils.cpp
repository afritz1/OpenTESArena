#include "ArenaEntityUtils.h"
#include "../Assets/ExeData.h"
#include "../Math/Random.h"
#include "../Stats/CharacterClassLibrary.h"

namespace
{
	int GetCreatureGoldChance(uint32_t lootChance)
	{
		return lootChance & 0xFF;
	}

	int GetCreatureMagicItemChance(uint32_t lootChance)
	{
		return (lootChance >> 8) & 0xFF;
	}

	int GetCreatureNonMagicWeaponOrArmorChance(uint32_t lootChance)
	{
		return (lootChance >> 16) & 0xFF;
	}

	int GetCreatureMagicWeaponOrArmorChance(uint32_t lootChance)
	{
		return (lootChance >> 24) & 0xFF;
	}
}

int ArenaEntityUtils::getBaseSpeed(int speedAttribute)
{
	return ((((speedAttribute * 20) / 256) * 256) / 256) + 20;
}

int ArenaEntityUtils::getCreatureGold(int creatureLevel, uint32_t creatureLootChance, Random &random)
{
	const int goldChance = GetCreatureGoldChance(creatureLootChance);

	const int firstRoll = 1 + random.next(100);
	if (firstRoll > goldChance)
	{
		return 0;
	}

	const int secondRoll = random.next(101);
	if (secondRoll < goldChance)
	{
		return 0;
	}

	const int goldAmount = (1 + random.next(10)) * (creatureLevel + 1);
	return goldAmount;
}

bool ArenaEntityUtils::getCreatureHasMagicItem(int creatureLevel, uint32_t creatureLootChance, Random &random)
{
	const bool isHighEnoughLevel = creatureLevel > 2;
	if (!isHighEnoughLevel)
	{
		return false;
	}
	
	const int itemChance = GetCreatureMagicItemChance(creatureLootChance);
	const int roll = 1 + random.next(100);
	return roll <= itemChance;
}

bool ArenaEntityUtils::getCreatureHasNonMagicWeaponOrArmor(uint32_t creatureLootChance, Random &random)
{
	const int itemChance = GetCreatureNonMagicWeaponOrArmorChance(creatureLootChance);
	const int roll = 1 + random.next(100);
	return roll <= itemChance;
}

bool ArenaEntityUtils::getCreatureHasMagicWeaponOrArmor(int creatureLevel, uint32_t creatureLootChance, Random &random)
{
	const bool isHighEnoughLevel = creatureLevel > 6;
	if (!isHighEnoughLevel)
	{
		return false;
	}

	const int itemChance = GetCreatureMagicWeaponOrArmorChance(creatureLootChance);
	const int roll = 1 + random.next(100);
	return roll <= itemChance;
}

int ArenaEntityUtils::getCreatureItemQualityLevel(int creatureLevel)
{
	return creatureLevel + 1;
}

int ArenaEntityUtils::getHumanEnemyGold(int charClassDefID, const ExeData &exeData, Random &random)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	
	int goldChanceIndex = 2;
	if (charClassDef.categoryID == CharacterClassDefinition::CATEGORY_ID_THIEF)
	{
		goldChanceIndex = 0;
	}
	else if (charClassDef.categoryID == CharacterClassDefinition::CATEGORY_ID_MAGE)
	{
		goldChanceIndex = 1;
	}

	const auto &goldChances = exeData.entities.humanEnemyGoldChances;
	DebugAssertIndex(goldChances, goldChanceIndex);
	const int goldChance = goldChances[goldChanceIndex];
	const int roll = 1 + random.next(100);
	if (roll >= goldChance)
	{
		return 0;
	}

	const int goldAmount = 1 + random.next(50);
	return goldAmount;
}
