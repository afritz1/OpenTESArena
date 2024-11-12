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

void AccessoryItemDefinition::init(const char *name, ItemMaterialDefinitionID materialDefID)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->materialDefID = materialDefID;
}

void ArmorItemDefinition::initLeather(const char *name)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->materialType = ArmorMaterialType::Leather;
	this->plateMaterialDefID = -1;
}

void ArmorItemDefinition::initChain(const char *name)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->materialType = ArmorMaterialType::Chain;
	this->plateMaterialDefID = -1;
}

void ArmorItemDefinition::initPlate(const char *name, ItemMaterialDefinitionID materialDefID)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->materialType = ArmorMaterialType::Plate;
	this->plateMaterialDefID = materialDefID;
}

void ConsumableItemDefinition::init(const char *name, const char *unidentifiedName)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	std::snprintf(std::begin(this->unidentifiedName), std::size(this->unidentifiedName), "%s", unidentifiedName);
}

void MiscItemDefinition::init(const char *name)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
}

void ShieldItemDefinition::init(const char *name)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
}

void TrinketItemDefinition::init(const char *name)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
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

void ArtifactItemDefinition::init(const char *flavorText, BufferView<const int> provinceIDs)
{
	std::snprintf(std::begin(this->flavorText), std::size(this->flavorText), "%s", flavorText);
	
	DebugAssert(provinceIDs.getCount() <= static_cast<int>(std::size(this->provinceIDs)));
	std::copy(provinceIDs.begin(), provinceIDs.end(), std::begin(this->provinceIDs));
	this->provinceCount = provinceIDs.getCount();
}

ItemDefinition::ItemDefinition()
{
	this->type = static_cast<ItemType>(-1);
	this->isArtifact = false;
}

void ItemDefinition::init(ItemType type)
{
	this->type = type;
	this->isArtifact = false;
}
