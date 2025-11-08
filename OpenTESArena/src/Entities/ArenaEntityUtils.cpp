#include <algorithm>

#include "ArenaEntityUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/ExeData.h"
#include "../Items/ItemDefinition.h"
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

ArenaValidLootSlots::ArenaValidLootSlots()
{
	std::fill(std::begin(this->slots), std::end(this->slots), false);
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

int ArenaEntityUtils::pickNonMagicArmor(int armorLevel, int baseMaterial, int specifiedItemID, const ExeData &exeData, Random &random)
{
	const auto &plateArmorQualities = exeData.equipment.plateArmorQualities;
	const auto &chainArmorQualities = exeData.equipment.chainArmorQualities;
	const auto &leatherArmorQualities = exeData.equipment.leatherArmorQualities;

	int initialItemID = -1;
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
	constexpr int invalidMaterialID = -1;
	constexpr int plateMaterialID = 0;
	constexpr int chainMaterialID = 1;
	constexpr int leatherMaterialID = 2;

	if (baseMaterial == invalidMaterialID || baseMaterial == plateMaterialID)
	{
		DebugAssertIndex(plateArmorQualities, initialItemID);
		for (int i = initialItemID; i < static_cast<int>(std::size(plateArmorQualities)); i++)
		{
			if (plateArmorQualities[i] <= armorLevel)
			{
				finalItemID = i;
				break;
			}
		}
	}
	else if (baseMaterial == chainMaterialID)
	{
		DebugAssertIndex(chainArmorQualities, initialItemID);
		for (int i = initialItemID; i < static_cast<int>(std::size(chainArmorQualities)); i++)
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
		for (int i = initialItemID; i < static_cast<int>(std::size(leatherArmorQualities)); i++)
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

int ArenaEntityUtils::pickNonMagicWeapon(int weaponLevel, int specifiedItemID, const ExeData &exeData, Random &random)
{
	DebugAssert(weaponLevel >= 1);
	const auto &weaponQualities = exeData.equipment.weaponQualities;
	constexpr int maximumWeaponQuality = 20;
	for (int i = 0; i < std::size(weaponQualities); i++)
	{
		DebugAssert(weaponQualities[i] <= maximumWeaponQuality);
	}
	const int weaponQualityCount = static_cast<int>(std::size(weaponQualities));

	int itemID = -1;
	do
	{
		if (specifiedItemID != -1)
		{
			itemID = specifiedItemID;
			weaponLevel = maximumWeaponQuality; // Breaks out of the loop
		}
		else
		{
			itemID = random.next(weaponQualityCount);
		}
	} while (weaponLevel < weaponQualities[itemID]);

	return itemID;
}

void ArenaEntityUtils::getCreatureNonMagicWeaponOrArmor(int creatureLevel, const ExeData &exeData, Random &random, int *outWeaponOrArmorID,
	bool *outIsArmor, ArmorMaterialType *outArmorMaterialType)
{
	int itemID = -1;
	bool isArmor = false;

	constexpr int itemCreationAttemptCount = 20;

	for (int i = 0; i < itemCreationAttemptCount; i++)
	{
		const int itemQualityLevel = ArenaEntityUtils::getCreatureItemQualityLevel(creatureLevel);
		const bool shouldPickArmor = random.nextBool();

		if (shouldPickArmor)
		{
			constexpr int baseMaterial = -1;
			constexpr int specifiedArmorID = -1;
			itemID = ArenaEntityUtils::pickNonMagicArmor(itemQualityLevel, baseMaterial, specifiedArmorID, exeData, random);
		}
		else
		{
			constexpr int specifiedWeaponID = -1;
			itemID = ArenaEntityUtils::pickNonMagicWeapon(itemQualityLevel, specifiedWeaponID, exeData, random);
		}

		// After picking an armor or weapon the original game calls a function for checking whether a class
		// can equip an item, using the byte value at +5 in the character data, which is the class ID for
		// human characters but seems to be an unused spell-related value for creatures. If the function
		// says the item can't be equipped, it is rejected. Non-spellcasting creatures have 0 for this value,
		// which is interpreted as the Mage, and so they only get Mage equipment (dagger, staff or buckler),
		// and some high-level creatures have values outside the range of class IDs, resulting in out-of-range
		// accesses.

		if (itemID >= 0)
		{
			isArmor = shouldPickArmor;
			break;
		}
	}

	*outWeaponOrArmorID = itemID;
	*outIsArmor = isArmor;
	*outArmorMaterialType = ArmorMaterialType::Plate;
}

int ArenaEntityUtils::getCreatureNonMagicWeaponOrArmorCondition(int maxCondition, const ExeData &exeData, Random &random)
{
	const auto &itemConditionChances = exeData.equipment.creatureItemConditionChances;
	const auto &itemConditionPercentages = exeData.equipment.creatureItemConditionPercentages;

	int roll = random.next(7);
	int condition = maxCondition;
	for (int i = 0; i < static_cast<int>(std::size(itemConditionChances)); i++)
	{
		if (itemConditionChances[i] >= roll)
		{
			DebugAssertIndex(itemConditionPercentages, i);
			const uint8_t conditionPercent = itemConditionPercentages[i];
			condition = std::max((maxCondition / 100) * conditionPercent, 1);
			break;
		}
	}

	return condition;
}

void ArenaEntityUtils::getCreatureMagicItem(int creatureLevel, const ExeData &exeData, Random &random, int *outItemID, bool *outIsPotion, ItemMaterialDefinitionID *outMaterialID, AttributeDefinitionID *outAttributeID, SpellDefinitionID *outSpellID)
{
	int itemID = -1;
	ItemMaterialDefinitionID materialID = -1;
	AttributeDefinitionID attributeID = -1;
	SpellDefinitionID spellID = -1;
	bool isPotion;

	if (random.nextBool())
	{
		isPotion = true;
		itemID = ArenaEntityUtils::pickPotion(random);
	}
	else
	{
		isPotion = false;
		const int quality = ArenaEntityUtils::getCreatureItemQualityLevel(creatureLevel);
		ArenaEntityUtils::pickMagicAccessoryOrTrinket(-1, quality, exeData, random, &itemID, &materialID, &attributeID, &spellID);
	}

	*outMaterialID = materialID;
	*outItemID = itemID;
	*outAttributeID = attributeID;
	*outIsPotion = isPotion;
	*outSpellID = spellID;
}

int ArenaEntityUtils::pickPotion(Random &random)
{
	const int numberOfPotionTypes = 15;
	return random.next(numberOfPotionTypes);
}

void ArenaEntityUtils::pickMagicAccessoryOrTrinket(int specifiedItemID, int quality, const ExeData &exeData, Random &random, int *outItemID, ItemMaterialDefinitionID *outMaterialID, AttributeDefinitionID *outAttributeID, SpellDefinitionID *outSpellID)
{
	int itemID = -1;
	ItemMaterialDefinitionID materialID = -1;
	AttributeDefinitionID attributeID = -1;
	SpellDefinitionID spellID = -1;

	const int type = random.next(3);
	if (type == 0)
	{
		ArenaEntityUtils::pickSpellCastingItem(-1, quality, exeData, random, &itemID, &spellID);
	}
	else if (type == 1)
	{
		ArenaEntityUtils::pickAttributeEnhancementItem(specifiedItemID, quality, exeData, random, &itemID, &attributeID);
	}
	else
	{
		ArenaEntityUtils::pickArmorClassItem(specifiedItemID, exeData, random, &itemID, &materialID);
	}

	*outItemID = itemID;
	*outMaterialID = materialID;
	*outAttributeID = attributeID;
	*outSpellID = spellID;
}

void ArenaEntityUtils::pickSpellCastingItem(int specifiedItemID, int quality, const ExeData &exeData, Random &random, int *outItemID, SpellDefinitionID *outSpellID) {
	const auto &spellcastingBaseItemChances = exeData.equipment.spellcastingItemCumulativeChances;
	const auto &spellcastingItemAttackSpellQualities = exeData.equipment.spellcastingItemAttackSpellQualities;
	const auto &spellcastingItemAttackSpellSpells = exeData.equipment.spellcastingItemAttackSpellSpells;
	const auto &spellcastingItemDefensiveSpellQualities = exeData.equipment.spellcastingItemDefensiveSpellQualities;
	const auto &spellcastingItemDefensiveSpellSpells = exeData.equipment.spellcastingItemDefensiveSpellSpells;
	const auto &spellcastingItemMiscSpellQualities = exeData.equipment.spellcastingItemMiscSpellQualities;
	const auto &spellcastingItemMiscSpellSpells = exeData.equipment.spellcastingItemMiscSpellSpells;

	int itemID = -1;
	SpellDefinitionID spellID = -1;

	if (specifiedItemID != -1)
	{
		itemID = specifiedItemID;
	}
	else
	{
		int roll = random.next(100);
		for (itemID = 0; spellcastingBaseItemChances[itemID] <= roll; itemID++) {
			DebugAssertIndex(spellcastingBaseItemChances, itemID);
		}
	}

	const int spellType = random.next(3);
	if (spellType == 0)
	{
		const int spellcastingItemAttackSpellQualitiesCount = static_cast<int>(std::size(spellcastingItemAttackSpellQualities));
		bool validSpellExists = false;
		for (int i = 0; i < spellcastingItemAttackSpellQualitiesCount; i++)
		{
			if (spellcastingItemAttackSpellQualities[i] <= quality)
			{
				validSpellExists = true;
				break;
			}
		}
		DebugAssert(validSpellExists);

		int spellIDIndex;
		do
		{
			spellIDIndex = random.next(spellcastingItemAttackSpellQualitiesCount);
		} while (quality < (spellcastingItemAttackSpellQualities[spellIDIndex]));

		spellID = spellcastingItemAttackSpellSpells[spellIDIndex];
	}
	else if (spellType == 1)
	{
		const int spellcastingItemDefensiveSpellQualitiesCount = static_cast<int>(std::size(spellcastingItemDefensiveSpellQualities));
		bool validSpellExists = false;
		for (int i = 0; i < spellcastingItemDefensiveSpellQualitiesCount; i++)
		{
			if (spellcastingItemDefensiveSpellQualities[i] <= quality)
			{
				validSpellExists = true;
				break;
			}
		}
		DebugAssert(validSpellExists);

		int spellIDIndex;
		do
		{
			spellIDIndex = random.next(spellcastingItemDefensiveSpellQualitiesCount);
		} while (quality < (spellcastingItemDefensiveSpellQualities[spellIDIndex]));

		spellID = spellcastingItemDefensiveSpellSpells[spellIDIndex];
	}
	else
	{
		const int spellcastingItemMiscSpellQualitiesCount = static_cast<int>(std::size(spellcastingItemMiscSpellQualities));
		bool validSpellExists = false;
		for (int i = 0; i < spellcastingItemMiscSpellQualitiesCount; i++)
		{
			if (spellcastingItemMiscSpellQualities[i] <= quality)
			{
				validSpellExists = true;
				break;
			}
		}
		DebugAssert(validSpellExists);

		int spellIDIndex;
		do
		{
			spellIDIndex = random.next(spellcastingItemMiscSpellQualitiesCount);
		} while (quality < (spellcastingItemMiscSpellQualities[spellIDIndex]));

		spellID = spellcastingItemMiscSpellSpells[spellIDIndex];
	}

	*outItemID = itemID;
	*outSpellID = spellID;
}

void ArenaEntityUtils::pickAttributeEnhancementItem(int specifiedItemID, int quality, const ExeData &exeData, Random &random, int *outItemID, AttributeDefinitionID *outAttributeID)
{
	const auto &enhancementBaseItemChances = exeData.equipment.enhancementItemCumulativeChances;
	int itemID = -1;
	AttributeDefinitionID attributeID = -1;
	const int attributeCount = 8;

	if (quality > 6)
	{
		if (specifiedItemID != -1)
		{
			itemID = specifiedItemID;
		}
		else
		{
			int roll = random.next(100);
			for (itemID = 0; enhancementBaseItemChances[itemID] <= roll; itemID++)
			{
				DebugAssertIndex(enhancementBaseItemChances, itemID);
			}
		}
		attributeID = random.next(attributeCount);
	}

	*outAttributeID = attributeID;
	*outItemID = itemID;
}

void ArenaEntityUtils::pickArmorClassItem(int specifiedItemID, const ExeData &exeData, Random &random, int *outItemID, ItemMaterialDefinitionID *outMaterialID)
{
	const auto &armorClassItemMaterialChances = exeData.equipment.armorClassItemMaterialChances;
	const int numberOfItemIDs = 4;

	int itemID = -1;
	if (specifiedItemID != -1)
	{
		itemID = specifiedItemID;
	}
	else
	{
		itemID = random.next(numberOfItemIDs);
	}

	int roll = random.next(24) + 76;
	ItemMaterialDefinitionID material = 0;
	for (; armorClassItemMaterialChances[material] <= roll; material++)
	{
		DebugAssertIndex(armorClassItemMaterialChances, material);
	}

	*outMaterialID = material + 3; // The first 3 materials aren't used
	*outItemID = itemID;
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

int ArenaEntityUtils::getLootValuesIndex(ArenaInteriorType interiorType)
{
	switch (interiorType)
	{
	case ArenaInteriorType::House:
		return ArenaEntityUtils::LOOT_VALUES_INDEX_HOUSE;
	case ArenaInteriorType::Palace:
		return ArenaEntityUtils::LOOT_VALUES_INDEX_PALACE;
	case ArenaInteriorType::Noble:
		return ArenaEntityUtils::LOOT_VALUES_INDEX_NOBLE;
	case ArenaInteriorType::Dungeon:
		return ArenaEntityUtils::LOOT_VALUES_INDEX_DUNGEON;
	case ArenaInteriorType::Crypt:
	case ArenaInteriorType::Tower:
		return ArenaEntityUtils::LOOT_VALUES_INDEX_CRYPT;
	default:
		return 0;
	}
}

ArenaValidLootSlots ArenaEntityUtils::getPopulatedLootSlots(int lootValuesIndex, const ExeData &exeData, Random &random)
{
	ArenaValidLootSlots lootSlots;

	for (int i = 0; i < ArenaValidLootSlots::COUNT; i++)
	{
		const int lootChanceIndex = (lootValuesIndex * 4) + i;
		DebugAssertIndex(exeData.items.lootChances, lootChanceIndex);
		const uint8_t lootChance = exeData.items.lootChances[lootChanceIndex];

		const int roll = random.next(100) + 1;
		lootSlots.slots[i] = roll <= lootChance;
	}

	return lootSlots;
}

int ArenaEntityUtils::getLootGoldAmount(int lootValuesIndex, const ExeData &exeData, Random &random, ArenaCityType cityType, int levelIndex)
{
	int goldAmount = 0;

	switch (lootValuesIndex)
	{
	case ArenaEntityUtils::LOOT_VALUES_INDEX_HOUSE:
	{
		goldAmount = random.next(9) + 2;
		if (cityType == ArenaCityType::Village)
		{
			goldAmount /= 2;
		}
		else
		{
			goldAmount *= 2;
		}

		break;
	}
	case ArenaEntityUtils::LOOT_VALUES_INDEX_PALACE:
	{
		const int cityTypeIndex = static_cast<int>(cityType);
		DebugAssertIndex(exeData.items.palaceGoldValues, cityTypeIndex);
		goldAmount = exeData.items.palaceGoldValues[cityTypeIndex];
		break;
	}
	case ArenaEntityUtils::LOOT_VALUES_INDEX_NOBLE:
	{
		goldAmount = random.next(9) + 2;
		if (cityType == ArenaCityType::Village)
		{
			goldAmount /= 2;
		}
		else
		{
			goldAmount *= 2;
		}

		goldAmount *= 10;
		break;
	}
	case ArenaEntityUtils::LOOT_VALUES_INDEX_DUNGEON:
	case ArenaEntityUtils::LOOT_VALUES_INDEX_CRYPT: // and TOWER
		goldAmount = (levelIndex * levelIndex) + (random.next(100) + 1);
		break;
	default:
		break;
	}

	return goldAmount;
}

void ArenaEntityUtils::getLootMagicItem(int lootValuesIndex, ArenaCityType cityType, int levelIndex, const ExeData &exeData, Random &random, int *outItemID, bool *outIsPotion, ItemMaterialDefinitionID *outMaterialID, AttributeDefinitionID *outAttributeID, SpellDefinitionID *outSpellID)
{
	int itemID = -1;
	ItemMaterialDefinitionID materialID = -1;
	AttributeDefinitionID attributeID = -1;
	bool isPotion;
	SpellDefinitionID spellID = -1;

	if (random.nextBool())
	{
		isPotion = true;
		itemID = pickPotion(random);
	}
	else
	{
		isPotion = false;
		const int quality = getLootItemQualityValue(lootValuesIndex, random, cityType, levelIndex);
		if (quality >= 3)
		{
			ArenaEntityUtils::pickMagicAccessoryOrTrinket(-1, quality, exeData, random, &itemID, &materialID, &attributeID, &spellID);
		}
	}
	
	*outItemID = itemID;
	*outMaterialID = materialID;
	*outAttributeID = attributeID;
	*outIsPotion = isPotion;
	*outSpellID = spellID;
}

int ArenaEntityUtils::getLootItemQualityValue(int lootValuesIndex, Random &random, ArenaCityType cityType, int levelIndex)
{
	int itemQualityLevel = 0;
	switch (lootValuesIndex)
	{
	case ArenaEntityUtils::LOOT_VALUES_INDEX_HOUSE:
		itemQualityLevel = random.next(5) + 1;
		break;
	case ArenaEntityUtils::LOOT_VALUES_INDEX_PALACE:
		if (cityType == ArenaCityType::CityState)
		{
			itemQualityLevel = 16;
		}
		else if (cityType == ArenaCityType::Town)
		{
			itemQualityLevel = 14;
		}
		else
		{
			itemQualityLevel = 12;
		}
		break;
	case ArenaEntityUtils::LOOT_VALUES_INDEX_NOBLE:
		itemQualityLevel = random.next(9) + 2;
		break;
	case ArenaEntityUtils::LOOT_VALUES_INDEX_DUNGEON:
		itemQualityLevel = 5 * (levelIndex + 1);
		break;
	case ArenaEntityUtils::LOOT_VALUES_INDEX_CRYPT: // and TOWER
		itemQualityLevel = 3 * levelIndex;
		break;
	default:
		break;
	}

	return itemQualityLevel;
}

void ArenaEntityUtils::getLootNonMagicWeaponOrArmor(const ExeData &exeData, Random &random, int *outWeaponOrArmorID, bool *outIsArmor,
	ArmorMaterialType *outArmorMaterialType)
{
	int itemID = -1;
	bool isArmor = false;

	// The original game gets itemQualityLevel with GetLootItemQualityValue but then overwrites it with 16
	const int itemQualityLevel = 16;
	const bool shouldPickArmor = random.nextBool();

	if (shouldPickArmor)
	{
		constexpr int baseMaterial = -1;
		constexpr int specifiedArmorID = -1;
		itemID = ArenaEntityUtils::pickNonMagicArmor(itemQualityLevel, baseMaterial, specifiedArmorID, exeData, random);
	}
	else
	{
		constexpr int specifiedWeaponID = -1;
		itemID = ArenaEntityUtils::pickNonMagicWeapon(itemQualityLevel, specifiedWeaponID, exeData, random);
	}

	if (itemID >= 0)
	{
		isArmor = shouldPickArmor;
	}

	*outWeaponOrArmorID = itemID;
	*outIsArmor = isArmor;
	*outArmorMaterialType = ArmorMaterialType::Plate;
}

int ArenaEntityUtils::getLootNonMagicWeaponOrArmorCondition(int lootValuesIndex, const ExeData &exeData, Random &random, int itemMaxHealth)
{
	const auto &itemConditionPercentages = exeData.equipment.lootItemConditionPercentages;
	const auto &itemConditionUsesFavorablePercentages = exeData.equipment.lootItemConditionUsesFavorablePercentages;

	DebugAssertIndex(itemConditionUsesFavorablePercentages, lootValuesIndex);
	const uint8_t itemConditionUsesFavorablePercentage = itemConditionUsesFavorablePercentages[lootValuesIndex];

	int lootConditionsIndex;
	if (itemConditionUsesFavorablePercentage != 0)
	{
		lootConditionsIndex = random.next(3) + 1;
		if (lootConditionsIndex == 3)
		{
			lootConditionsIndex = 2;
		}
	}
	else
	{
		lootConditionsIndex = random.next(3);
	}

	DebugAssertIndex(itemConditionPercentages, lootConditionsIndex);
	int condition = itemConditionPercentages[lootConditionsIndex] * itemMaxHealth / 100;
	if (condition == 0)
	{
		condition = 1;
	}

	return condition;
}

std::string ArenaEntityUtils::getArmorNameFromItemID(int itemID, const ExeData &exeData)
{
	// Currently this is just for armor in loot or on creatures, which is always plate.
	DebugAssertIndex(exeData.equipment.plateArmorNames, itemID);
	std::string name = exeData.equipment.plateArmorNames[itemID];
	return name;
}

std::string ArenaEntityUtils::getWeaponNameFromItemID(int itemID, const ExeData &exeData)
{
	DebugAssertIndex(exeData.equipment.weaponNames, itemID);
	return exeData.equipment.weaponNames[itemID];
}
