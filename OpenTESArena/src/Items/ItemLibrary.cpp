#include <algorithm>

#include "ArenaItemUtils.h"
#include "ItemLibrary.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

void ItemLibrary::init(const ExeData &exeData)
{
	const Span<const std::string> accessoryNames = exeData.equipment.enhancementItemNames;
	const Span<const std::string> materialNames = exeData.equipment.materialNames;
	const Span<const std::string> attributeNames = exeData.equipment.enhancementItemAttributeNames;

	for (int i = 0; i < accessoryNames.getCount(); i++)
	{
		for (ItemMaterialDefinitionID materialID = 3; materialID < materialNames.getCount(); materialID++) // The first 3 materials aren't used
		{
			ItemDefinition itemDef;
			itemDef.init(ItemType::Accessory, i);
			const std::string fullName = materialNames[materialID] + " " + accessoryNames[i];
			const int basePrice = ArenaItemUtils::getArmorClassMagicItemBasePrice(materialID, exeData);
			itemDef.accessory.init(fullName.c_str(), accessoryNames[i].c_str(), materialID, -1, basePrice);
			this->itemDefs.emplace_back(std::move(itemDef));
		}

		for (PrimaryAttributeID attributeID = 0; attributeID < attributeNames.getCount(); attributeID++)
		{
			ItemDefinition itemDef;
			itemDef.init(ItemType::Accessory, i);
			const std::string fullName = accessoryNames[i] + " " + attributeNames[attributeID];
			const int basePrice = ArenaItemUtils::getAttributeEnhancementMagicItemBasePrice(i, attributeID, exeData);
			itemDef.accessory.init(fullName.c_str(), accessoryNames[i].c_str(), -1, attributeID, basePrice);
			this->itemDefs.emplace_back(std::move(itemDef));
		}
	}

	constexpr double kgDivisor = ArenaItemUtils::KilogramsDivisor;
	constexpr int armorCount = 7; // Ignores shields at end.
	const Span<const std::string> leatherArmorNames(exeData.equipment.leatherArmorNames, armorCount);
	const Span<const std::string> chainArmorNames(exeData.equipment.chainArmorNames, armorCount);
	const Span<const std::string> plateArmorNames(exeData.equipment.plateArmorNames, armorCount);
	const Span<const std::string> armorNames(exeData.equipment.armorNames, armorCount); // Requires an associated material.
	const Span<const uint16_t> leatherArmorWeights(exeData.equipment.leatherArmorWeights, armorCount);
	const Span<const uint16_t> chainArmorWeights(exeData.equipment.chainArmorWeights, armorCount);
	const Span<const uint16_t> plateArmorWeights(exeData.equipment.plateArmorWeights, armorCount);

	for (int i = 0; i < leatherArmorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor, i);

		const std::string &leatherArmorName = leatherArmorNames[i];
		const int weightOriginal = leatherArmorWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / kgDivisor;
		itemDef.armor.initLeather(leatherArmorName.c_str(), weightKg);
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	for (int i = 0; i < chainArmorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor, i);

		const std::string &chainArmorName = chainArmorNames[i];
		const int weightOriginal = chainArmorWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / kgDivisor;
		itemDef.armor.initChain(chainArmorName.c_str(), weightKg);
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	for (int i = 0; i < plateArmorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor, i);

		const std::string &plateArmorName = plateArmorNames[i];
		const int weightOriginal = plateArmorWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / kgDivisor;
		itemDef.armor.initPlate(plateArmorName.c_str(), weightKg, -1);
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	/* @todo:These are for plate armor with a material. Commented out until those are ready.
	for (int i = 0; i < armorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor);

		const int weightOriginal = plateArmorWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / kgDivisor;
		itemDef.armor.initPlate(armorNames[i].c_str(), weightKg, -1); // @todo: for loop over all materials
		this->itemDefs.emplace_back(std::move(itemDef));
	}*/

	const Span<const std::string> potionNames = exeData.equipment.potionNames;
	const std::string &unidentifiedPotionName = exeData.equipment.unidentifiedPotionName;
	for (int i = 0; i < potionNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Consumable, i);
		itemDef.consumable.init(potionNames[i].c_str(), unidentifiedPotionName.c_str());
		this->itemDefs.emplace_back(std::move(itemDef));
	}
	
	const Span<const std::string> mainQuestItemNames = exeData.quests.mainQuestItemNames;
	for (int i = 0; i < mainQuestItemNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Misc, -1);
		itemDef.misc.init(mainQuestItemNames[i].c_str());
		this->itemDefs.emplace_back(std::move(itemDef));
	}
	
	constexpr int shieldCount = 4;
	const Span<const std::string> shieldNames(exeData.equipment.armorNames + armorCount, shieldCount);
	const Span<const uint16_t> shieldWeights(exeData.equipment.plateArmorWeights + armorCount, shieldCount);
	for (int i = 0; i < shieldNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Shield, armorCount + i);

		const int weightOriginal = shieldWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / kgDivisor;
		itemDef.shield.init(shieldNames[i].c_str(), weightKg);
		this->itemDefs.emplace_back(std::move(itemDef));
	}
	
	const Span<const std::string> trinketNames = exeData.equipment.spellcastingItemNames;
	const Span<const std::string> attackSpellNames = exeData.equipment.spellcastingItemAttackSpellNames;
	const Span<const std::string> defensiveSpellNames = exeData.equipment.spellcastingItemDefensiveSpellNames;
	const Span<const std::string> miscSpellNames = exeData.equipment.spellcastingItemMiscSpellNames;
	const auto &spellcastingItemAttackSpellSpells = exeData.equipment.spellcastingItemAttackSpellSpells;
	const auto &spellcastingItemDefensiveSpellSpells = exeData.equipment.spellcastingItemDefensiveSpellSpells;
	const auto &spellcastingItemMiscSpellSpells = exeData.equipment.spellcastingItemMiscSpellSpells;

	for (int i = 0; i < trinketNames.getCount(); i++)
	{
		for (int spellIndex = 0; spellIndex < attackSpellNames.getCount(); spellIndex++)
		{
			ItemDefinition itemDef;
			itemDef.init(ItemType::Trinket, i);
			const std::string fullName = trinketNames[i] + " " + attackSpellNames[spellIndex];
			itemDef.trinket.init(fullName.c_str(), trinketNames[i].c_str(), spellcastingItemAttackSpellSpells[spellIndex]);
			this->itemDefs.emplace_back(std::move(itemDef));
		}

		for (int spellIndex = 0; spellIndex < defensiveSpellNames.getCount(); spellIndex++)
		{
			ItemDefinition itemDef;
			itemDef.init(ItemType::Trinket, i);
			const std::string fullName = trinketNames[i] + " " + defensiveSpellNames[spellIndex];
			itemDef.trinket.init(fullName.c_str(), trinketNames[i].c_str(), spellcastingItemDefensiveSpellSpells[spellIndex]);
			this->itemDefs.emplace_back(std::move(itemDef));
		}

		for (int spellIndex = 0; spellIndex < miscSpellNames.getCount(); spellIndex++)
		{
			ItemDefinition itemDef;
			itemDef.init(ItemType::Trinket, i);
			const std::string fullName = trinketNames[i] + " " + miscSpellNames[spellIndex];
			itemDef.trinket.init(fullName.c_str(), trinketNames[i].c_str(), spellcastingItemMiscSpellSpells[spellIndex]);
			this->itemDefs.emplace_back(std::move(itemDef));
		}
	}
	
	const Span<const std::string> weaponNames = exeData.equipment.weaponNames;
	const Span<const uint16_t> weaponWeights = exeData.equipment.weaponWeights;
	const Span<const uint8_t> weaponBasePrices = exeData.equipment.weaponBasePrices;
	const Span<const std::pair<uint8_t, uint8_t>> weaponDamages = exeData.equipment.weaponDamages;
	const Span<const uint8_t> weaponHandednesses = exeData.equipment.weaponHandednesses;
	for (int i = 0; i < weaponNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Weapon, i);

		const char *weaponName = weaponNames[i].c_str();
		const int weightOriginal = weaponWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / kgDivisor;
		const int basePrice = weaponBasePrices[i];
		const int damageMin = weaponDamages[i].first;
		const int damageMax = weaponDamages[i].second;
		const int handCount = weaponHandednesses[i];
		const ItemMaterialDefinitionID materialDefID = -1; // @todo: for loop over all materials

		const auto rangedWeaponIDsBegin = std::begin(ArenaItemUtils::RangedWeaponIDs);
		const auto rangedWeaponIDsEnd = std::end(ArenaItemUtils::RangedWeaponIDs);
		const bool isRanged = std::find(rangedWeaponIDsBegin, rangedWeaponIDsEnd, i) != rangedWeaponIDsEnd;

		if (!isRanged)
		{
			itemDef.weapon.initMelee(weaponName, weightKg, basePrice, damageMin, damageMax, handCount, materialDefID);
		}
		else
		{
			itemDef.weapon.initRanged(weaponName, weightKg, basePrice, damageMin, damageMax, materialDefID);
		}
		
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	// Used with loot containers. Player's gold is just a character sheet value.
	ItemDefinition goldItemDef;
	goldItemDef.init(ItemType::Gold, -1);
	goldItemDef.gold.init(exeData.items.goldPiece.c_str(), exeData.items.bagOfGoldPieces.c_str());
	this->itemDefs.emplace_back(std::move(goldItemDef));
}

int ItemLibrary::getCount() const
{
	return static_cast<int>(this->itemDefs.size());
}

const ItemDefinition &ItemLibrary::getDefinition(int index) const
{
	DebugAssertIndex(this->itemDefs, index);
	return this->itemDefs[index];
}

int ItemLibrary::getFirstDefinitionIndexIf(const ItemLibraryPredicate &predicate) const
{
	int index = -1;
	for (int i = 0; i < this->getCount(); i++)
	{
		if (predicate(this->getDefinition(i)))
		{
			index = i;
			break;
		}
	}

	return index;
}

std::vector<int> ItemLibrary::getDefinitionIndicesIf(const ItemLibraryPredicate &predicate) const
{
	std::vector<int> indices;
	for (int i = 0; i < this->getCount(); i++)
	{
		if (predicate(this->getDefinition(i)))
		{
			indices.emplace_back(i);
		}
	}

	return indices;
}

const ItemDefinition &ItemLibrary::getGoldDefinition() const
{
	const ItemDefinition *itemDef = nullptr;
	for (int i = 0; i < this->getCount(); i++)
	{
		const ItemDefinition &curItemDef = this->itemDefs[i];
		if (curItemDef.type == ItemType::Gold)
		{
			itemDef = &curItemDef;
			break;
		}
	}

	DebugAssertMsg(itemDef != nullptr, "Couldn't find gold item definition.");
	return *itemDef;
}

ItemDefinitionID ItemLibrary::getGoldDefinitionID() const
{
	ItemDefinitionID itemDefID = -1;
	for (int i = 0; i < this->getCount(); i++)
	{
		const ItemDefinition &curItemDef = this->itemDefs[i];
		if (curItemDef.type == ItemType::Gold)
		{
			itemDefID = i;
			break;
		}
	}

	DebugAssertMsg(itemDefID != -1, "Couldn't find gold item definition ID.");
	return itemDefID;
}
