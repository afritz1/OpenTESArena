#include "ArenaEntityUtils.h"
#include "../Math/Random.h"

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
