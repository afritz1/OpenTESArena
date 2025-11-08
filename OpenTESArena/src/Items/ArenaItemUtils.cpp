#include <algorithm>
#include <iterator>

#include "ArenaItemUtils.h"
#include "ItemDefinition.h"
#include "../Math/Random.h"

bool ArenaItemUtils::isFistsWeapon(int weaponID)
{
	return weaponID == ArenaItemUtils::FistsWeaponID;
}

bool ArenaItemUtils::isRangedWeapon(int weaponID)
{
	const auto rangedWeaponsBegin = std::begin(ArenaItemUtils::RangedWeaponIDs);
	const auto rangedWeaponsEnd = std::end(ArenaItemUtils::RangedWeaponIDs);
	return std::find(rangedWeaponsBegin, rangedWeaponsEnd, weaponID) != rangedWeaponsEnd;
}

int ArenaItemUtils::getArmorClassMagicItemBasePrice(int materialID, const ExeData &exeData)
{
	const Span<const uint16_t> materialPriceMultipliers = exeData.equipment.armorClassItemMaterialPriceMultipliers;
	return materialPriceMultipliers[materialID] * 50;
}

int ArenaItemUtils::getAttributeEnhancementMagicItemBasePrice(int baseItemID, int attributeID, const ExeData &exeData)
{
	const Span<const uint16_t> attributeItemBasePrices = exeData.equipment.enhancementItemBasePrices;
	const Span<const uint16_t> attributeEnhancementPrices = exeData.equipment.enhancementItemAttributePrices;
	return attributeItemBasePrices[baseItemID] + attributeEnhancementPrices[attributeID];
}

ArmorMaterialType ArenaItemUtils::getRandomArmorMaterialType(Random &random)
{
	return static_cast<ArmorMaterialType>(random.next(ARMOR_MATERIAL_TYPE_COUNT));
}
