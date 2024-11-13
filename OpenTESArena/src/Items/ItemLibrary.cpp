#include "ItemLibrary.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

namespace
{
	constexpr double KilogramsDivisor = 256.0; // @todo: move this to some ArenaItemUtils
}

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

	constexpr int armorCount = 7; // Ignores shields at end.
	const BufferView<const std::string> leatherArmorNames(exeData.equipment.leatherArmorNames.data(), armorCount);
	const BufferView<const std::string> chainArmorNames(exeData.equipment.chainArmorNames.data(), armorCount);
	const BufferView<const std::string> armorNames(exeData.equipment.armorNames.data(), armorCount); // Requires an associated material.
	const BufferView<const uint16_t> leatherArmorWeights(exeData.equipment.leatherArmorWeights.data(), armorCount);
	const BufferView<const uint16_t> chainArmorWeights(exeData.equipment.chainArmorWeights.data(), armorCount);
	const BufferView<const uint16_t> plateArmorWeights(exeData.equipment.plateArmorWeights.data(), armorCount);
	//const BufferView<const std::string> plateArmorNames = exeData.equipment.plateArmorNames; // Not sure 'ordinary' plate exists in-game
	for (int i = 0; i < leatherArmorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor);

		const int weightOriginal = leatherArmorWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / KilogramsDivisor;
		itemDef.armor.initLeather(leatherArmorNames[i].c_str(), weightKg);
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	for (int i = 0; i < chainArmorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor);

		const int weightOriginal = chainArmorWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / KilogramsDivisor;
		itemDef.armor.initChain(chainArmorNames[i].c_str(), weightKg);
		this->itemDefs.emplace_back(std::move(itemDef));
	}

	for (int i = 0; i < armorNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Armor);

		const int weightOriginal = plateArmorWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / KilogramsDivisor;
		itemDef.armor.initPlate(armorNames[i].c_str(), weightKg, -1); // @todo: for loop over all materials
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
	
	constexpr int shieldCount = 4;
	const BufferView<const std::string> shieldNames(exeData.equipment.armorNames.data() + armorCount, shieldCount);
	const BufferView<const uint16_t> shieldWeights(exeData.equipment.plateArmorWeights.data() + armorCount, shieldCount);
	for (int i = 0; i < shieldNames.getCount(); i++)
	{
		ItemDefinition itemDef;
		itemDef.init(ItemType::Shield);

		const int weightOriginal = shieldWeights[i];
		const double weightKg = static_cast<double>(weightOriginal) / KilogramsDivisor;
		itemDef.shield.init(shieldNames[i].c_str(), weightKg);
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
		const double weightKg = static_cast<double>(weightOriginal) / KilogramsDivisor;
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

int ItemLibrary::getCount() const
{
	return static_cast<int>(this->itemDefs.size());
}

const ItemDefinition &ItemLibrary::getDefinition(int index) const
{
	DebugAssertIndex(this->itemDefs, index);
	return this->itemDefs[index];
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