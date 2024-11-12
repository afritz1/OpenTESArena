#include "ItemLibrary.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

void ItemLibrary::init(const ExeData &exeData)
{
	const BufferView<const std::string> accessoryNames = exeData.equipment.enhancementItemNames;
	for (int i = 0; i < accessoryNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Accessory);
		itemDef.accessory.init(accessoryNames[i].c_str(), -1); // @todo: for loop over all materials
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	const BufferView<const std::string> leatherArmorNames(exeData.equipment.leatherArmorNames.data(), 7);
	const BufferView<const std::string> chainArmorNames(exeData.equipment.chainArmorNames.data(), 7);
	const BufferView<const std::string> armorNames(exeData.equipment.armorNames.data(), 7); // Requires an associated material.
	//const BufferView<const std::string> plateArmorNames = exeData.equipment.plateArmorNames; // Not sure 'ordinary' plate exists in-game
	for (int i = 0; i < leatherArmorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor);
		itemDef.armor.initLeather(leatherArmorNames[i].c_str());
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	for (int i = 0; i < chainArmorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor);
		itemDef.armor.initChain(chainArmorNames[i].c_str());
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	for (int i = 0; i < armorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor);
		itemDef.armor.initPlate(armorNames[i].c_str(), -1); // @todo: for loop over all materials
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	const BufferView<const std::string> potionNames = exeData.equipment.potionNames;
	const std::string &unidentifiedPotionName = exeData.equipment.unidentifiedPotionName;
	for (int i = 0; i < potionNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Consumable);
		itemDef.consumable.init(potionNames[i].c_str(), unidentifiedPotionName.c_str());
		this->itemDefs.emplace_back(std::move(itemDef));
	}
	
	const BufferView<const std::string> mainQuestItemNames = exeData.quests.mainQuestItemNames;
	for (int i = 0; i < mainQuestItemNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Misc);
		itemDef.misc.init(mainQuestItemNames[i].c_str());
		this->itemDefs.emplace_back(std::move(itemDef));
	}
	
	const BufferView<const std::string> shieldNames(exeData.equipment.armorNames.data() + 7, 4);
	for (int i = 0; i < shieldNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Shield);
		itemDef.shield.init(shieldNames[i].c_str());
		this->itemDefs.emplace_back(std::move(itemDef));
	}
	
	const BufferView<const std::string> trinketNames = exeData.equipment.spellcastingItemNames;
	for (int i = 0; i < trinketNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Trinket);
		itemDef.trinket.init(trinketNames[i].c_str());
		this->itemDefs.emplace_back(std::move(itemDef));
	}
	
	const BufferView<const std::string> weaponNames = exeData.equipment.weaponNames;
	const BufferView<const uint16_t> weaponWeights = exeData.equipment.weaponWeights;
	const BufferView<const uint8_t> weaponBasePrices = exeData.equipment.weaponBasePrices;
	const BufferView<const std::pair<uint8_t, uint8_t>> weaponDamages = exeData.equipment.weaponDamages;
	const BufferView<const uint8_t> weaponHandednesses = exeData.equipment.weaponHandednesses;
	for (int i = 0; i < weaponNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Weapon);

		const char *weaponName = weaponNames[i].c_str();
		const int weightOriginal = weaponWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / 256.0;
		const int basePrice = weaponBasePrices[i];
		const int damageMin = weaponDamages[i].first;
		const int damageMax = weaponDamages[i].second;
		const int handCount = weaponHandednesses[i];
		const ItemMaterialDefinitionID materialDefID = -1; // @todo: for loop over all materials
		const bool isRanged = (i == 16) || (i == 17);

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
}

int ItemLibrary::getItemDefCount() const
{
	return static_cast<int>(this->itemDefs.size());
}

const ItemDefinition &ItemLibrary::getItemDef(int index) const
{
	DebugAssertIndex(this->itemDefs, index);
	return this->itemDefs[index];
}

std::vector<int> ItemLibrary::getItemDefIndicesIf(const ItemLibraryPredicate &predicate) const
{
	std::vector<int> indices;
	for (int i = 0; i < this->getItemDefCount(); i++)
	{
		if (predicate(this->getItemDef(i)))
		{
			indices.emplace_back(i);
		}
	}

	return indices;
}
