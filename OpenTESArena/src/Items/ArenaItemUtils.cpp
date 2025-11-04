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

ArmorMaterialType ArenaItemUtils::getRandomArmorMaterialType(Random &random)
{
	return static_cast<ArmorMaterialType>(random.next(ARMOR_MATERIAL_TYPE_COUNT));
}
