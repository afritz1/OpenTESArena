#include <algorithm>
#include <iterator>

#include "ArenaItemUtils.h"

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
