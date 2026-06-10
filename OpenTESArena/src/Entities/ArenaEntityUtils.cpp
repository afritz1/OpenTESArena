#include <algorithm>

#include "ArenaEntityUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/ExeData.h"
#include "../Items/ItemDefinition.h"
#include "../Math/ArenaMathUtils.h"
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

ArenaEntitySpawnPoint::ArenaEntitySpawnPoint()
{
	this->x = 0;
	this->z = 0;
	this->tileIndex = 0;
}

ArenaEntitySpawnPoint::ArenaEntitySpawnPoint(int x, int z, uint16_t tileIndex)
{
	this->x = x;
	this->z = z;
	this->tileIndex = tileIndex;
}

ArenaEnemyEncounter::ArenaEnemyEncounter()
{
	this->count = 0;
	this->id = 0;
	this->level = 0;
}

int ArenaEntityUtils::getBaseSpeed(int speedAttribute)
{
	return ((((speedAttribute * 20) / 256) * 256) / 256) + 20;
}

int ArenaEntityUtils::getCreatureGold(int creatureLevel, uint32_t creatureLootChance, ArenaRandom &random)
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

bool ArenaEntityUtils::getCreatureHasMagicItem(int creatureLevel, uint32_t creatureLootChance, ArenaRandom &random)
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

bool ArenaEntityUtils::getCreatureHasNonMagicWeaponOrArmor(uint32_t creatureLootChance, ArenaRandom &random)
{
	const int itemChance = GetCreatureNonMagicWeaponOrArmorChance(creatureLootChance);
	const int roll = 1 + random.next(100);
	return roll <= itemChance;
}

bool ArenaEntityUtils::getCreatureHasMagicWeaponOrArmor(int creatureLevel, uint32_t creatureLootChance, ArenaRandom &random)
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

void ArenaEntityUtils::getHumanEnemyArmor(int classNumber, int level, const ExeData &exeData, ArenaRandom &random, Span<int> outArmorIDs, ArmorMaterialType *outArmorMaterialType)
{
	DebugAssert(outArmorIDs.getCount() == 7);

	constexpr int armorForbiddenID = 3;

	Span<const uint8_t> allowedArmors = exeData.charClasses.allowedArmors;
	if (allowedArmors[classNumber] == armorForbiddenID)
	{
		return;
	}

	constexpr int plateMaterialID = 0;
	constexpr int chainMaterialID = 1;
	constexpr int leatherMaterialID = 2;

	int armorMaterial = leatherMaterialID;
	int upgradeChance = 0;
	int classID = exeData.charClasses.classNumbersToIDs[classNumber];

	constexpr int warriorClassID = 0x10;
	constexpr int knightClassID = 0x11;

	// Warriors and Knights start at chain and can upgrade to plate.
	if ((classID == warriorClassID) || (classID == knightClassID))
	{
		armorMaterial = chainMaterialID;
		upgradeChance = level * 10;
	}
	else
	{
		// These classes start at leather and can upgrade to chain.
		Span<const uint8_t> chainCapableClasses = exeData.entities.humanEnemyChainCapableClasses;

		const bool canUpgradeToChain = std::ranges::find(chainCapableClasses, classID) != chainCapableClasses.end();

		if (canUpgradeToChain)
		{
			upgradeChance = level * 10;
		}
	}

	// Off-by-one bonus chance as per original game. Roll returns 0-99. If the roll is <= upgradeChance then success,
	// so the chance is (level * 10) + 1 percent.
	const int roll = random.next(100);
	if (roll <= upgradeChance)
	{
		armorMaterial--;
	}

	constexpr int dummyQualityThreshold = 1;
	for (int i = 0; i < 4; i++)
	{
		// The original executable has unreachable code to instead pick a magical or named plate material armor piece
		// if random.next(100) < 2. This also applies for the secondary slots below.
		outArmorIDs[i] = ArenaEntityUtils::pickNonMagicArmor(dummyQualityThreshold, armorMaterial, exeData.entities.humanEnemyPrimaryArmorSlots[i], exeData, random);
	}

	constexpr int minRequiredLevelForSecondaryArmor = 5;
	constexpr int secondaryArmorRequiredLevelStep = 5;
	int requiredLevel = minRequiredLevelForSecondaryArmor;
	for (int i = 0; i < 3; i++)
	{
		if (level < requiredLevel)
		{
			break;
		}

		outArmorIDs[i + 4] = ArenaEntityUtils::pickNonMagicArmor(dummyQualityThreshold, armorMaterial, exeData.entities.humanEnemySecondaryArmorSlots[i], exeData, random);
		requiredLevel += secondaryArmorRequiredLevelStep;
	}

	if (armorMaterial == leatherMaterialID)
	{
		*outArmorMaterialType = ArmorMaterialType::Leather;
	}
	else if (armorMaterial == chainMaterialID)
	{
		*outArmorMaterialType = ArmorMaterialType::Chain;
	}
	else
	{
		*outArmorMaterialType = ArmorMaterialType::Plate;
	}
}

void ArenaEntityUtils::getHumanEnemyWeapon(int classNumber, const ExeData &exeData, ArenaRandom &random, int *outWeaponID)
{
	constexpr int archerID = 13;
	constexpr int staffWeaponID = 0;
	constexpr int longBowWeaponID = 17;
	constexpr int mageClassNumber = 0;

	int weaponID = longBowWeaponID;
	Span<const int> allowedWeaponIndices = exeData.charClasses.allowedWeaponsIndices;
	Span<const std::vector<uint8_t>> allowedWeaponLists = exeData.charClasses.allowedWeaponsLists;

	while (true)
	{
		if (classNumber != archerID)
		{
			int allowedWeaponIndex = allowedWeaponIndices[classNumber];

			if (allowedWeaponIndex == -1)
			{
				weaponID = random.next(15) + 2;
			}
			else
			{
				const int allowedWeaponListSize = static_cast<int>(allowedWeaponLists[allowedWeaponIndex].size());
				const int allowedWeaponListRandomIndex = random.next(allowedWeaponListSize);
				weaponID = allowedWeaponLists[allowedWeaponIndex][allowedWeaponListRandomIndex];
			}
		}

		if (random.next(100) < 3)
		{
			// TODO: Pick magic/material weapon
		}
		else
		{
			constexpr int dummyQualityThreshold = 1;
			weaponID = ArenaEntityUtils::pickNonMagicWeapon(dummyQualityThreshold, weaponID, exeData, random);
		}

		// Non-mages reroll staffs.
		if (weaponID != staffWeaponID || classNumber == mageClassNumber)
		{
			break;
		}
	}

	*outWeaponID = weaponID;
}

void ArenaEntityUtils::getHumanEnemyShield(int classNumber, const ExeData &exeData, ArenaRandom &random, int weaponID, int *outShieldID)
{
	constexpr int invalidID = -1;
	constexpr int plateMaterialID = 0;

	int shieldID = invalidID;

	if (exeData.equipment.weaponHandednesses[weaponID] != 2)
	{
		Span<const int> allowedShieldsIndices = exeData.charClasses.allowedShieldsIndices;
		Span<const std::vector<uint8_t>> allowedShieldsLists = exeData.charClasses.allowedShieldsLists;

		const int allowedShieldsIndex = allowedShieldsIndices[classNumber];
		const int allowedShieldsListSize = static_cast<int>(allowedShieldsLists[allowedShieldsIndex].size());
		const bool classHasAllowedShields = allowedShieldsListSize > 0;
		if (classHasAllowedShields)
		{
			const int allowedShieldsListRandomIndex = random.next(allowedShieldsListSize);
			shieldID = allowedShieldsLists[allowedShieldsIndex][allowedShieldsListRandomIndex];

			// The original executable has unreachable code to instead pick a magical or named plate material armor piece
			// if random.next(100) < 2.
			constexpr int dummyQualityThreshold = 1;
			shieldID = pickNonMagicArmor(dummyQualityThreshold, plateMaterialID, shieldID, exeData, random);
		}
	}

	*outShieldID = shieldID;
}

int ArenaEntityUtils::pickNonMagicArmor(int itemQualityThreshold, int baseMaterial, int specifiedItemID, const ExeData &exeData, ArenaRandom &random)
{
	constexpr int invalidID = -1;
	constexpr int plateMaterialID = 0;
	constexpr int chainMaterialID = 1;
	constexpr int leatherMaterialID = 2;

	Span<const uint8_t> qualities;

	// Matches an original game bug where random armor generation always picks plate armor if no material is specified.
	if ((baseMaterial == invalidID) || (baseMaterial == plateMaterialID))
	{
		qualities = exeData.equipment.plateArmorQualities;
	}
	else if (baseMaterial == chainMaterialID)
	{
		qualities = exeData.equipment.chainArmorQualities;
	}
	else
	{
		qualities = exeData.equipment.leatherArmorQualities;
	}

	// If an armor ID is specified, the game forces the quality threshold to 20 and only does the quality check for the specified ID.
	if (specifiedItemID != invalidID)
	{
		itemQualityThreshold = 20;

		DebugAssertIndex(qualities, specifiedItemID);

		if (qualities[specifiedItemID] <= itemQualityThreshold)
		{
			return specifiedItemID;
		}

		return invalidID;
	}

	// Random generation starts from a random armor ID and scans upward until a valid item is found.
	const int initialItemID = random.next(11);

	for (int itemID = initialItemID; itemID < qualities.getCount(); itemID++)
	{
		if (qualities[itemID] <= itemQualityThreshold)
		{
			return itemID;
		}
	}

	return invalidID;
}

int ArenaEntityUtils::pickNonMagicWeapon(int weaponQualityThreshold, int specifiedItemID, const ExeData &exeData, ArenaRandom &random)
{
	DebugAssert(weaponQualityThreshold >= 1);
	constexpr int maximumWeaponQuality = 20;
	const Span<const uint8_t> weaponQualities = exeData.equipment.weaponQualities;
	const int weaponQualityCount = weaponQualities.getCount();

	for (int i = 0; i < weaponQualityCount; i++)
	{
		DebugAssert(weaponQualities[i] <= maximumWeaponQuality);
	}

	int itemID = -1;
	do
	{
		if (specifiedItemID != -1)
		{
			itemID = specifiedItemID;
			weaponQualityThreshold = maximumWeaponQuality; // Breaks out of the loop
		}
		else
		{
			itemID = random.next(weaponQualityCount);
		}
	} while (weaponQualityThreshold < weaponQualities[itemID]);

	return itemID;
}

void ArenaEntityUtils::getCreatureNonMagicWeaponOrArmor(int creatureLevel, const ExeData &exeData, ArenaRandom &random, int *outWeaponOrArmorID,
	bool *outIsArmor, ArmorMaterialType *outArmorMaterialType)
{
	int itemID = -1;
	bool isArmor = false;

	constexpr int itemCreationAttemptCount = 20;

	for (int i = 0; i < itemCreationAttemptCount; i++)
	{
		const int itemQualityThreshold = ArenaEntityUtils::getCreatureItemQualityThreshold(creatureLevel);
		const bool shouldPickArmor = random.nextBool();

		if (shouldPickArmor)
		{
			constexpr int baseMaterial = -1;
			constexpr int specifiedArmorID = -1;
			itemID = ArenaEntityUtils::pickNonMagicArmor(itemQualityThreshold, baseMaterial, specifiedArmorID, exeData, random);
		}
		else
		{
			constexpr int specifiedWeaponID = -1;
			itemID = ArenaEntityUtils::pickNonMagicWeapon(itemQualityThreshold, specifiedWeaponID, exeData, random);
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

int ArenaEntityUtils::getCreatureNonMagicWeaponOrArmorCondition(int maxCondition, const ExeData &exeData, ArenaRandom &random)
{
	const Span<const uint8_t> itemConditionChances = exeData.equipment.creatureItemConditionChances;
	const Span<const uint8_t> itemConditionPercentages = exeData.equipment.creatureItemConditionPercentages;

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

void ArenaEntityUtils::getCreatureMagicItem(int creatureLevel, const ExeData &exeData, ArenaRandom &random, int *outItemID, bool *outIsPotion,
	ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID)
{
	int itemID = -1;
	ItemMaterialDefinitionID materialID = -1;
	PrimaryAttributeID attributeID = -1;
	SpellID spellID = -1;
	bool isPotion;

	if (random.nextBool())
	{
		isPotion = true;
		itemID = ArenaEntityUtils::pickPotion(random);
	}
	else
	{
		isPotion = false;
		const int qualityThreshold = ArenaEntityUtils::getCreatureItemQualityThreshold(creatureLevel);
		ArenaEntityUtils::pickMagicAccessoryOrTrinket(-1, qualityThreshold, exeData, random, &itemID, &materialID, &attributeID, &spellID);
	}

	*outMaterialID = materialID;
	*outItemID = itemID;
	*outAttributeID = attributeID;
	*outIsPotion = isPotion;
	*outSpellID = spellID;
}

int ArenaEntityUtils::pickPotion(ArenaRandom &random)
{
	const int numberOfPotionTypes = 15;
	return random.next(numberOfPotionTypes);
}

void ArenaEntityUtils::pickMagicAccessoryOrTrinket(int specifiedItemID, int qualityThreshold, const ExeData &exeData, ArenaRandom &random, int *outItemID,
	ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID)
{
	int itemID = -1;
	ItemMaterialDefinitionID materialID = -1;
	PrimaryAttributeID attributeID = -1;
	SpellID spellID = -1;

	const int type = random.next(3);
	if (type == 0)
	{
		ArenaEntityUtils::pickSpellCastingItem(-1, qualityThreshold, exeData, random, &itemID, &spellID);
	}
	else if (type == 1)
	{
		ArenaEntityUtils::pickAttributeEnhancementItem(specifiedItemID, qualityThreshold, exeData, random, &itemID, &attributeID);
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

void ArenaEntityUtils::pickSpellCastingItem(int specifiedItemID, int qualityThreshold, const ExeData &exeData, ArenaRandom &random, int *outItemID, SpellID *outSpellID)
{
	const Span<const uint8_t> spellcastingBaseItemChances = exeData.equipment.spellcastingItemCumulativeChances;
	const Span<const uint8_t> spellcastingItemAttackSpellQualities = exeData.equipment.spellcastingItemAttackSpellQualities;
	const Span<const uint8_t> spellcastingItemAttackSpellSpells = exeData.equipment.spellcastingItemAttackSpellSpells;
	const Span<const uint8_t> spellcastingItemDefensiveSpellQualities = exeData.equipment.spellcastingItemDefensiveSpellQualities;
	const Span<const uint8_t> spellcastingItemDefensiveSpellSpells = exeData.equipment.spellcastingItemDefensiveSpellSpells;
	const Span<const uint8_t> spellcastingItemMiscSpellQualities = exeData.equipment.spellcastingItemMiscSpellQualities;
	const Span<const uint8_t> spellcastingItemMiscSpellSpells = exeData.equipment.spellcastingItemMiscSpellSpells;

	int itemID = -1;
	SpellID spellID = -1;

	if (specifiedItemID != -1)
	{
		itemID = specifiedItemID;
	}
	else
	{
		int roll = random.next(100);
		for (itemID = 0; spellcastingBaseItemChances[itemID] <= roll; itemID++)
		{
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
			if (spellcastingItemAttackSpellQualities[i] <= qualityThreshold)
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
		} while (qualityThreshold < (spellcastingItemAttackSpellQualities[spellIDIndex]));

		spellID = spellcastingItemAttackSpellSpells[spellIDIndex];
	}
	else if (spellType == 1)
	{
		const int spellcastingItemDefensiveSpellQualitiesCount = static_cast<int>(std::size(spellcastingItemDefensiveSpellQualities));
		bool validSpellExists = false;
		for (int i = 0; i < spellcastingItemDefensiveSpellQualitiesCount; i++)
		{
			if (spellcastingItemDefensiveSpellQualities[i] <= qualityThreshold)
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
		} while (qualityThreshold < (spellcastingItemDefensiveSpellQualities[spellIDIndex]));

		spellID = spellcastingItemDefensiveSpellSpells[spellIDIndex];
	}
	else
	{
		const int spellcastingItemMiscSpellQualitiesCount = static_cast<int>(std::size(spellcastingItemMiscSpellQualities));
		bool validSpellExists = false;
		for (int i = 0; i < spellcastingItemMiscSpellQualitiesCount; i++)
		{
			if (spellcastingItemMiscSpellQualities[i] <= qualityThreshold)
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
		} while (qualityThreshold < (spellcastingItemMiscSpellQualities[spellIDIndex]));

		spellID = spellcastingItemMiscSpellSpells[spellIDIndex];
	}

	*outItemID = itemID;
	*outSpellID = spellID;
}

void ArenaEntityUtils::pickAttributeEnhancementItem(int specifiedItemID, int quality, const ExeData &exeData, ArenaRandom &random, int *outItemID, PrimaryAttributeID *outAttributeID)
{
	const Span<const uint8_t> enhancementBaseItemChances = exeData.equipment.enhancementItemCumulativeChances;
	int itemID = -1;
	PrimaryAttributeID attributeID = -1;
	const int attributeCount = 8;

	if (quality > 6)
	{
		if (specifiedItemID != -1)
		{
			itemID = specifiedItemID;
		}
		else
		{
			const int roll = random.next(100);
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

void ArenaEntityUtils::pickArmorClassItem(int specifiedItemID, const ExeData &exeData, ArenaRandom &random, int *outItemID, ItemMaterialDefinitionID *outMaterialID)
{
	const Span<const uint8_t> armorClassItemMaterialChances = exeData.equipment.armorClassItemMaterialChances;
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
	for (material = 0; armorClassItemMaterialChances[material] <= roll; material++)
	{
		DebugAssertIndex(armorClassItemMaterialChances, material);
	}

	*outMaterialID = material + 3; // The first 3 materials aren't used
	*outItemID = itemID;
}

int ArenaEntityUtils::getCreatureItemQualityThreshold(int creatureLevel)
{
	return creatureLevel + 1;
}

int ArenaEntityUtils::getHumanEnemyGold(int charClassDefID, const ExeData &exeData, ArenaRandom &random)
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

	const Span<const uint8_t> goldChances = exeData.entities.humanEnemyGoldChances;
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

int ArenaEntityUtils::getHumanEnemyExperience(int level, int charClassDefID, const ExeData &exeData)
{
	const CharacterClassDefinition &charClassDef = CharacterClassLibrary::getInstance().getDefinition(charClassDefID);
	const int index = charClassDef.categoryID;
	const Span<const uint8_t> humanoidExpMultipliers = exeData.entities.humanEnemyExpMultipliers;
	const int experience = (level * level) * humanoidExpMultipliers[index];
	return experience;
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

ArenaValidLootSlots ArenaEntityUtils::getPopulatedLootSlots(int lootValuesIndex, const ExeData &exeData, ArenaRandom &random)
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

int ArenaEntityUtils::getLootGoldAmount(int lootValuesIndex, const ExeData &exeData, ArenaRandom &random, ArenaCityType cityType, int levelIndex)
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

void ArenaEntityUtils::getLootMagicItem(int lootValuesIndex, ArenaCityType cityType, int levelIndex, const ExeData &exeData, ArenaRandom &random,
	int *outItemID, bool *outIsPotion, ItemMaterialDefinitionID *outMaterialID, PrimaryAttributeID *outAttributeID, SpellID *outSpellID)
{
	int itemID = -1;
	ItemMaterialDefinitionID materialID = -1;
	PrimaryAttributeID attributeID = -1;
	bool isPotion;
	SpellID spellID = -1;

	if (random.nextBool())
	{
		isPotion = true;
		itemID = ArenaEntityUtils::pickPotion(random);
	}
	else
	{
		isPotion = false;

		const int qualityThreshold = ArenaEntityUtils::getLootItemQualityThreshold(lootValuesIndex, random, cityType, levelIndex);
		if (qualityThreshold >= 3)
		{
			ArenaEntityUtils::pickMagicAccessoryOrTrinket(-1, qualityThreshold, exeData, random, &itemID, &materialID, &attributeID, &spellID);
		}
	}

	*outItemID = itemID;
	*outMaterialID = materialID;
	*outAttributeID = attributeID;
	*outIsPotion = isPotion;
	*outSpellID = spellID;
}

int ArenaEntityUtils::getLootItemQualityThreshold(int lootValuesIndex, ArenaRandom &random, ArenaCityType cityType, int levelIndex)
{
	int itemQualityThreshold = 0;
	switch (lootValuesIndex)
	{
	case ArenaEntityUtils::LOOT_VALUES_INDEX_HOUSE:
		itemQualityThreshold = random.next(5) + 1;
		break;
	case ArenaEntityUtils::LOOT_VALUES_INDEX_PALACE:
		if (cityType == ArenaCityType::CityState)
		{
			itemQualityThreshold = 16;
		}
		else if (cityType == ArenaCityType::Town)
		{
			itemQualityThreshold = 14;
		}
		else
		{
			itemQualityThreshold = 12;
		}
		break;
	case ArenaEntityUtils::LOOT_VALUES_INDEX_NOBLE:
		itemQualityThreshold = random.next(9) + 2;
		break;
	case ArenaEntityUtils::LOOT_VALUES_INDEX_DUNGEON:
		itemQualityThreshold = 5 * (levelIndex + 1);
		break;
	case ArenaEntityUtils::LOOT_VALUES_INDEX_CRYPT: // and TOWER
		itemQualityThreshold = 3 * levelIndex;
		break;
	default:
		break;
	}

	return itemQualityThreshold;
}

void ArenaEntityUtils::getLootNonMagicWeaponOrArmor(const ExeData &exeData, ArenaRandom &random, int *outWeaponOrArmorID, bool *outIsArmor,
	ArmorMaterialType *outArmorMaterialType)
{
	int itemID = -1;
	bool isArmor = false;

	// The original game gets itemQualityThreshold with GetLootItemQualityThreshold but then overwrites it with 16
	const int itemQualityThreshold = 16;
	const bool shouldPickArmor = random.nextBool();

	if (shouldPickArmor)
	{
		constexpr int baseMaterial = -1;
		constexpr int specifiedArmorID = -1;
		itemID = ArenaEntityUtils::pickNonMagicArmor(itemQualityThreshold, baseMaterial, specifiedArmorID, exeData, random);
	}
	else
	{
		constexpr int specifiedWeaponID = -1;
		itemID = ArenaEntityUtils::pickNonMagicWeapon(itemQualityThreshold, specifiedWeaponID, exeData, random);
	}

	if (itemID >= 0)
	{
		isArmor = shouldPickArmor;
	}

	*outWeaponOrArmorID = itemID;
	*outIsArmor = isArmor;
	*outArmorMaterialType = ArmorMaterialType::Plate;
}

int ArenaEntityUtils::getLootNonMagicWeaponOrArmorCondition(int lootValuesIndex, const ExeData &exeData, ArenaRandom &random, int itemMaxHealth)
{
	const Span<const uint8_t> itemConditionPercentages = exeData.equipment.lootItemConditionPercentages;
	const Span<const uint8_t> itemConditionUsesFavorablePercentages = exeData.equipment.lootItemConditionUsesFavorablePercentages;

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

bool ArenaEntityUtils::doGuardsAppearForViolence(int playerLevel, ArenaRandom &random)
{
	const int chance = std::max((playerLevel * -10) + 100, 50);
	return random.next(100) < chance;
}

bool ArenaEntityUtils::doGuardsAppearForTheft(int thievingSkill, ArenaRandom &random)
{
	return random.next(100) < (100 - thievingSkill);
}

int ArenaEntityUtils::getGuardType(const ExeData &exeData, ArenaRandom &random)
{
	Span<const uint8_t> guardTypeChances = exeData.entities.guardTypeChances;

	const int roll = random.next(100);

	int guardType = 0;
	while (guardTypeChances[guardType] < roll)
	{
		guardType++;
		DebugAssertIndex(exeData.entities.guardTypeChances, guardType);
	}

	return guardType;
}

int ArenaEntityUtils::getGuardLevel(ArenaCityType cityType, int tierBonus, const ExeData &exeData, ArenaRandom &random)
{
	const int cityTypeIndex = static_cast<int>(cityType);
	DebugAssertIndex(exeData.entities.guardLevelMinimums, cityTypeIndex);
	const int minimumLevel = exeData.entities.guardLevelMinimums[cityTypeIndex] + tierBonus;
	return random.next(3) + minimumLevel;
}

int ArenaEntityUtils::getNumberOfGuardsToSpawn(ArenaRandom &random)
{
	return random.next(3) + 1;
}

void ArenaEntityUtils::getGuardArmor(int guardType, const ExeData &exeData, ArenaRandom &random, Span<int> outArmorIDs, ArmorMaterialType *outArmorMaterialType)
{
	DebugAssert(outArmorIDs.getCount() == 4);

	constexpr int plateMaterialID = 0;
	constexpr int chainMaterialID = 1;
	constexpr int leatherMaterialID = 2;

	const Span<const uint8_t> guardArmorIDs = exeData.entities.guardArmorIDs;
	for (int i = 0; i < outArmorIDs.getCount(); i++)
	{
		constexpr int dummyQualityThreshold = 1;
		outArmorIDs[i] = ArenaEntityUtils::pickNonMagicArmor(dummyQualityThreshold, guardType, guardArmorIDs[i], exeData, random);
	}

	if (guardType == leatherMaterialID)
	{
		*outArmorMaterialType = ArmorMaterialType::Leather;
	}
	else if (guardType == chainMaterialID)
	{
		*outArmorMaterialType = ArmorMaterialType::Chain;
	}
	else
	{
		*outArmorMaterialType = ArmorMaterialType::Plate;
	}
}

int ArenaEntityUtils::getGuardWeapon(const ExeData &exeData, ArenaRandom &random)
{
	constexpr int dummyQualityThreshold = 1;
	constexpr int longSwordID = 5;
	return ArenaEntityUtils::pickNonMagicWeapon(dummyQualityThreshold, longSwordID, exeData, random);
}

int ArenaEntityUtils::getGuardShield(int guardType, const ExeData &exeData, ArenaRandom &random)
{
	constexpr int dummyQualityThreshold = 1;
	constexpr int plateMaterialID = 0;
	const Span<const uint8_t> guardShieldIDs = exeData.entities.guardShieldIDs;
	return ArenaEntityUtils::pickNonMagicArmor(dummyQualityThreshold, plateMaterialID, guardShieldIDs[guardType], exeData, random);
}

void ArenaEntityUtils::getEncounterParameters(int currentEnvironmentType, int currentBuildingType, bool playerTrespassing, bool playerResting,
	int terrainType, int hour, int dayOfYear, const ExeData &exeData, int *outEncounterChance, int *outEncounterTableIndex)
{
	constexpr int cityEnvironment = 0;
	constexpr int wildernessEnvironment = 1;
	constexpr int buildingEnvironment = 4;

	constexpr int magesGuildBuilding = 7;
	constexpr int cryptBuilding = 8;

	Span<const uint8_t> enemyEncounterChances = exeData.entities.enemyEncounterChances;

	int encounterChance = 0;
	int encounterTableIndex = 7;
	int encounterChancesIndex;
	const bool isNightTime = (hour <= 5) || (hour >= 18);
	bool allowNightEncounters = isNightTime;

	if (currentEnvironmentType == buildingEnvironment)
	{
		if (currentBuildingType < cryptBuilding)
		{
			if (playerTrespassing)
			{
				if (currentBuildingType != magesGuildBuilding)
				{
					encounterTableIndex = 6;
				}

				allowNightEncounters = false;
				encounterChance = 20;
			}
		}
		else if (currentBuildingType == cryptBuilding)
		{
			if (playerResting)
			{
				encounterChancesIndex = 7;
				encounterChance = enemyEncounterChances[encounterChancesIndex];
				allowNightEncounters = false;
			}
		}
		else // Tower
		{
			encounterChancesIndex = 3;
			if (!playerResting)
			{
				encounterChance = enemyEncounterChances[encounterChancesIndex];
			}
			else
			{
				encounterChancesIndex += 5;
				encounterChance = enemyEncounterChances[encounterChancesIndex];
			}

			allowNightEncounters = false;
		}
	}
	else if (currentEnvironmentType == cityEnvironment || currentEnvironmentType == wildernessEnvironment)
	{
		if (currentEnvironmentType == wildernessEnvironment)
		{
			encounterTableIndex = terrainType;
		}
		else
		{
			encounterTableIndex = 6;
		}

		encounterChancesIndex = currentEnvironmentType;
		if (allowNightEncounters)
		{
			encounterChancesIndex += 5;
		}

		encounterChance = enemyEncounterChances[encounterChancesIndex];

		if (currentEnvironmentType == cityEnvironment)
		{
			// City environment adds 5 to the chances index at night but then turns off this bool.
			// If it didn't, encounterTableIndex would become 13 below and no encounter would happen.
			allowNightEncounters = false;
		}
	}
	else // Main quest dungeon or random dungeon
	{
		encounterChancesIndex = currentEnvironmentType;
		if (playerResting)
		{
			encounterChancesIndex += 5;
		}

		encounterChance = enemyEncounterChances[encounterChancesIndex];
		allowNightEncounters = false;
	}

	if (allowNightEncounters)
	{
		encounterTableIndex += 7;
	}

	if (encounterTableIndex >= 13)
	{
		encounterChance = 0;
	}

	// Override encounterChance for night time for specific holidays.
	// One day later than the dates in holidayDates are used.
	constexpr int dayAfterTalesAndTallow = 243;
	constexpr int dayAfterWitchesFestival = 283;

	if (encounterChance != 0 && isNightTime && (dayOfYear == dayAfterTalesAndTallow || dayOfYear == dayAfterWitchesFestival))
	{
		encounterChance = 25;
	}

	*outEncounterChance = encounterChance;
	*outEncounterTableIndex = encounterTableIndex;
}

int ArenaEntityUtils::rollWeightedEncounterLevel(int encounterLevel, ArenaRandom &random)
{
	int rolledValue = ArenaMathUtils::rollBoundedDice(encounterLevel * 2, random);
	if (rolledValue > encounterLevel)
	{
		rolledValue = (encounterLevel * 2) - rolledValue;
	}

	return rolledValue;
}

ArenaEnemyEncounter ArenaEntityUtils::chooseEncounterEnemy(int encounterTableIndex, int encounterLevel, int playerLevel, bool mainQuestEncounter,
	ArenaRandom &random, const ExeData &exeData)
{
	constexpr int maxEncounterScalingLevel = 20;
	encounterLevel = std::min(encounterLevel + 1, maxEncounterScalingLevel);
	encounterLevel = encounterLevel + random.next(4);

	while (true)
	{
		const int r = ArenaEntityUtils::rollWeightedEncounterLevel(encounterLevel, random);
		if (r >= (encounterLevel / 2))
		{
			encounterLevel = std::max(r, 1);
			break;
		}

		encounterLevel *= 2;
	}

	if (playerLevel == 0 && encounterLevel >= 2)
	{
		encounterLevel = random.next(3);
	}

	if (mainQuestEncounter && encounterLevel < playerLevel)
	{
		encounterLevel = playerLevel - random.next(3);
	}

	const Span<const uint8_t> chosenEncounterTableIndices = exeData.entities.enemyEncounterTableIndices[encounterTableIndex];
	int matches[] = { -1, -1 };
	int count = 0;
	for (int i = 0; (i < 85) && (count < 2); i++)
	{
		if (chosenEncounterTableIndices[i] == encounterLevel)
		{
			matches[count] = i;
			count++;
		}
	}

	DebugAssert(count == 2);
	const int chosen = matches[random.next() & 1];
	DebugAssert(chosen != -1);

	const Span<const uint8_t> chosenEncounterTable = exeData.entities.enemyEncounterTable[chosen];

	ArenaEnemyEncounter chosenEnemyEncounter;
	chosenEnemyEncounter.count = chosenEncounterTable[0];
	chosenEnemyEncounter.id = chosenEncounterTable[1];
	chosenEnemyEncounter.level = chosenEncounterTable[2];
	return chosenEnemyEncounter;
}

ArenaEntitySpawnPoint ArenaEntityUtils::findRandomSpawnLocationAroundPlayer(int16_t playerX, int16_t playerZ, /*const uint16_t *map1, const uint16_t *floorMap, uint16_t invalidFloorThreshold, const std::array<uint16_t, 23> &occupiedTiles,*/ ArenaRandom &random)
{
	int shift = 6;

	while (true)
	{
		int attempts = 0;

		while (attempts < 40)
		{
			const int16_t randomXDelta = static_cast<int16_t>(static_cast<int16_t>(random.next()) >> shift);
			int16_t x = playerX + randomXDelta;
			const int16_t randomZDelta = static_cast<int16_t>(static_cast<int16_t>(random.next()) >> shift);
			int16_t z = playerZ + randomZDelta;

			x = snapToTileCenter(x);
			z = snapToTileCenter(z);

			const uint16_t tileIndex = makeTileIndex(x, z);
			const uint16_t playerTileIndex = makeTileIndex(playerX, playerZ);

			bool failed = false;

			if (tileIndex == playerTileIndex)
			{
				failed = true;
			}/*
			else if (ArenaEntityUtils::isTileOccupied(tileIndex, occupiedTiles))
			{
				failed = true;
			}
			else if (!ArenaEntityUtils::isSpawnTileValid(tileIndex, map1, floorMap, invalidFloorThreshold))
			{
				failed = true;
			}*/

			if (!failed)
			{
				return ArenaEntitySpawnPoint(x, z, tileIndex);
			}

			attempts++;
		}

		// Expand search radius
		shift >>= 1;
		DebugAssert(shift > 0);
	}

	DebugUnhandledReturn(ArenaEntitySpawnPoint);
}

bool ArenaEntityUtils::isTileOccupied(uint16_t tileIndex, Span<const uint16_t> occupiedTiles)
{
	DebugAssert(occupiedTiles.getCount() == 23);

	for (const uint16_t occupied : occupiedTiles)
	{
		if (occupied == tileIndex)
		{
			return true;
		}
	}

	return false;
}

bool ArenaEntityUtils::isSpawnTileValid(uint16_t tileIndex, Span<const uint16_t> map1, Span<const uint16_t> flor, uint16_t invalidFloorThreshold)
{
	const bool isWallOrObjectPresent = map1[tileIndex >> 1] != 0;
	if (isWallOrObjectPresent)
	{
		return false;
	}

	const bool isFloorValid = flor[tileIndex >> 1] < invalidFloorThreshold;
	if (!isFloorValid)
	{
		return false;
	}

	return true;
}

int ArenaEntityUtils::snapToTileCenter(int16_t coord)
{
	return (coord & ~127) + 64;
}

uint16_t ArenaEntityUtils::makeTileIndex(int16_t x, int16_t z)
{
	const int16_t tileX = x >> 7;
	const int16_t tileZ = z >> 7;
	return static_cast<uint16_t>((tileZ << 8) + (tileX << 1));
}
