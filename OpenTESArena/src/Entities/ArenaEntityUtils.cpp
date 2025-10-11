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

	// The original game rolls a second time against the chance for gold (which would otherwise simply
	// function as a % chance of gold) and reverses the comparison the second time. This makes gold
	// unlikely on high-level creatures, who were probably supposed to have high chances of gold.
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

int ArenaEntityUtils::getCreatureNonMagicWeaponOrArmor(int creatureLevel, const ExeData& exeData, Random& random)
{
	int itemID = -1;

	// Make up to 20 attempts to create a valid item
	for (int i = 0; i < 20; i++)
	{
		if (random.next(2) == 0)
		{
			itemID = pickNonMagicArmor(getCreatureItemQualityLevel(creatureLevel), -1, -1, exeData, random);
		}
		else
		{
			itemID = pickNonMagicWeapon(getCreatureItemQualityLevel(creatureLevel), -1, exeData, random);
		}

		// After picking an armor or weapon the original game calls a function for checking whether a class
		// can equip an item, using the byte value at +5 in the character data, which is the class ID for
		// human characters but seems to be an unused spell-related value for creatures. If the function
		// says the item can't be equipped, it is rejected. Non-spellcasting creatures have 0 for this value,
		// which is interpreted as the Mage, and so they only get Mage equipment (dagger, staff or buckler),
		// and some high-level creatures have values outside the range of class IDs, resulting in out-of-range
		// accesses.
		
		// If a valid item was found, stop
		if (itemID != -1)
			break;
	}

	return itemID; // @todo: Also return the base material (plate, chain or leather)
}

int ArenaEntityUtils::getCreatureNonMagicWeaponOrArmorCondition(int maxCondition, const ExeData& exeData, Random& random)
{
	const auto& itemConditionChances = exeData.equipment.itemConditionChances;
	const auto& itemConditionPercentages = exeData.equipment.itemConditionPercentages;

	int roll = random.next(7);
	int condition = maxCondition;
	for (int i = 0; i < sizeof(itemConditionChances); i++)
	{
		if (itemConditionChances[i] >= roll)
		{
			DebugAssertIndex(itemConditionPercentages, i);
			condition = (maxCondition / 100) * itemConditionPercentages[i];
			if (condition == 0) {
				condition = 1;
			}
			break;
		}
	}

	return condition;
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

int ArenaEntityUtils::pickNonMagicArmor(int armorLevel, int baseMaterial, int specifiedItemID, const ExeData& exeData, Random& random)
{
	const auto& plateArmorQualities = exeData.equipment.plateArmorQualities;
	const auto& chainArmorQualities = exeData.equipment.chainArmorQualities;
	const auto& leatherArmorQualities = exeData.equipment.leatherArmorQualities;

	int initialItemID;
	int finalItemID = -1;
	if (specifiedItemID != -1)
	{
		initialItemID = specifiedItemID;
		armorLevel = 20;
	}
	else
	{
		initialItemID = random.next(11);
	}

	// The original game picks a random value from 0 to 2 (plate, chain or leather) here, but it doesn't
	// use the result, instead overriding it with 0 (plate) when baseMaterial is -1 or using the specified
	// material when it is not -1. It probably was supposed to use the random value for the -1 case, which
	// is used for treasure piles and armor found on creatures.
	if (baseMaterial == -1 || baseMaterial == 0)
	{
		DebugAssertIndex(plateArmorQualities, initialItemID);
		for (int i = initialItemID; i < sizeof(plateArmorQualities); i++)
		{
			if (plateArmorQualities[i] <= armorLevel)
			{
				finalItemID = i;
				break;
			}
		}
	}
	else if (baseMaterial == 1)
	{
		DebugAssertIndex(chainArmorQualities, initialItemID);
		for (int i = initialItemID; i < sizeof(chainArmorQualities); i++)
		{
			if (chainArmorQualities[i] <= armorLevel)
			{
				finalItemID = i;
				break;
			}
		}
	}
	else
	{
		DebugAssertIndex(leatherArmorQualities, initialItemID);
		for (int i = initialItemID; i < sizeof(leatherArmorQualities); i++)
		{
			if (leatherArmorQualities[i] <= armorLevel)
			{
				finalItemID = i;
				break;
			}
		}
	}

	return finalItemID;
}

int ArenaEntityUtils::pickNonMagicWeapon(int weaponLevel, int specifiedItemID, const ExeData& exeData, Random& random)
{
	const auto& weaponQualities = exeData.equipment.weaponQualities;
	int initialItemID;
	int finalItemID = -1;
	if (specifiedItemID != -1)
	{
		initialItemID = specifiedItemID;
		weaponLevel = 20;
	}
	else
	{
		initialItemID = random.next(sizeof(weaponQualities));
	}

	for (int i = initialItemID; i < sizeof(weaponQualities); i++)
	{
		if (weaponQualities[i] <= weaponLevel)
		{
			finalItemID = i;
			break;
		}
	}

	return finalItemID;
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
