#include <algorithm>
#include <cstring>

#include "ItemDefinition.h"

#include "components/debug/Debug.h"

ItemConditionDefinition::ItemConditionDefinition()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
	this->current = 0;
	this->max = 0;
	this->degradeRate = 0;
}

void ItemConditionDefinition::init(const char *name, int current, int max, int degradeRate)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->current = current;
	this->max = max;
	this->degradeRate = degradeRate;
}

ItemMaterialDefinition::ItemMaterialDefinition()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
	this->ratingMultiplier = 0;
	this->conditionMultiplier = 0;
	this->weightMultiplier = 0;
}

void ItemMaterialDefinition::init(const char *name, int ratingMultiplier, int conditionMultiplier, int weightMultiplier)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->ratingMultiplier = ratingMultiplier;
	this->conditionMultiplier = conditionMultiplier;
	this->weightMultiplier = weightMultiplier;
}

void AccessoryItemDefinition::init(const char *name, const char *unidentifiedName, ItemMaterialDefinitionID materialDefID, PrimaryAttributeID attributeID, int basePrice)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	std::snprintf(std::begin(this->unidentifiedName), std::size(this->unidentifiedName), "%s", unidentifiedName);
	this->materialDefID = materialDefID;
	this->attributeID = attributeID;
	this->basePrice = basePrice;
}

void ArmorItemDefinition::initLeather(const char *name, double weight)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->weight = weight;
	this->materialType = ArmorMaterialType::Leather;
	this->plateMaterialDefID = -1;
}

void ArmorItemDefinition::initChain(const char *name, double weight)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->weight = weight;
	this->materialType = ArmorMaterialType::Chain;
	this->plateMaterialDefID = -1;
}

void ArmorItemDefinition::initPlate(const char *name, double weight, ItemMaterialDefinitionID materialDefID)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->weight = weight;
	this->materialType = ArmorMaterialType::Plate;
	this->plateMaterialDefID = materialDefID;
}

void ConsumableItemDefinition::init(const char *name, const char *unidentifiedName)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	std::snprintf(std::begin(this->unidentifiedName), std::size(this->unidentifiedName), "%s", unidentifiedName);
}

void GoldItemDefinition::init(const char *nameSingular, const char *namePlural)
{
	std::snprintf(std::begin(this->nameSingular), std::size(this->nameSingular), "%s", nameSingular);
	std::snprintf(std::begin(this->namePlural), std::size(this->namePlural), "%s", namePlural);
}

void MiscItemDefinition::init(const char *name)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
}

void ShieldItemDefinition::init(const char *name, double weight)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->weight = weight;
}

void TrinketItemDefinition::init(const char *name, const char* unidentifiedName, SpellDefinitionID spellID)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->spellID = spellID;
}

void WeaponItemDefinition::initMelee(const char *name, double weight, int basePrice, int damageMin, int damageMax, int handCount, ItemMaterialDefinitionID materialDefID)
{
	DebugAssert((handCount == 1) || (handCount == 2));
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->weight = weight;
	this->basePrice = basePrice;
	this->damageMin = damageMin;
	this->damageMax = damageMax;
	this->handCount = handCount;
	this->isRanged = false;
	this->materialDefID = materialDefID;
}

void WeaponItemDefinition::initRanged(const char *name, double weight, int basePrice, int damageMin, int damageMax, ItemMaterialDefinitionID materialDefID)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->weight = weight;
	this->basePrice = basePrice;
	this->damageMin = damageMin;
	this->damageMax = damageMax;
	this->handCount = 2;
	this->isRanged = true;
	this->materialDefID = materialDefID;
}

ArtifactItemDefinition::ArtifactItemDefinition()
{
	std::fill(std::begin(this->flavorText), std::end(this->flavorText), '\0');
	std::fill(std::begin(this->provinceIDs), std::end(this->provinceIDs), -1);
	this->provinceCount = 0;
}

void ArtifactItemDefinition::init(const char *flavorText, Span<const int> provinceIDs)
{
	std::snprintf(std::begin(this->flavorText), std::size(this->flavorText), "%s", flavorText);
	
	DebugAssert(provinceIDs.getCount() <= static_cast<int>(std::size(this->provinceIDs)));
	std::copy(provinceIDs.begin(), provinceIDs.end(), std::begin(this->provinceIDs));
	this->provinceCount = provinceIDs.getCount();
}

ItemDefinition::ItemDefinition()
{
	this->type = static_cast<ItemType>(-1);
	this->originalItemID = -1;
	this->isArtifact = false;
}

void ItemDefinition::init(ItemType type, int originalItemID)
{
	this->type = type;
	this->originalItemID = originalItemID;
	this->isArtifact = false;
}

std::string ItemDefinition::getDisplayName(int stackAmount) const
{
	// @todo eventually this will need stack counts from ItemInstance, so may as well move this there sometime

	switch (this->type)
	{
	case ItemType::Accessory:
		return this->accessory.name;
	case ItemType::Armor:
		return this->armor.name;
	case ItemType::Consumable:
		return this->consumable.name;
	case ItemType::Gold:
		return (stackAmount == 1) ? this->gold.nameSingular : this->gold.namePlural;
	case ItemType::Misc:
		return this->misc.name;
	case ItemType::Shield:
		return this->shield.name;
	case ItemType::Trinket:
		return this->trinket.name;
	case ItemType::Weapon:
		return this->weapon.name;
	default:
		DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(this->type)));
	}
}

double ItemDefinition::getWeight() const
{
	switch (this->type)
	{
	case ItemType::Accessory:
		return 0.0;
	case ItemType::Armor:
		return this->armor.weight;
	case ItemType::Consumable:
		return 0.0;
	case ItemType::Gold:
		return 0.0;
	case ItemType::Misc:
		return 0.0;
	case ItemType::Shield:
		return this->shield.weight;
	case ItemType::Trinket:
		return 0.0;
	case ItemType::Weapon:
		return this->weapon.weight;
	default:
		DebugUnhandledReturnMsg(double, std::to_string(static_cast<int>(this->type)));
	}
}
