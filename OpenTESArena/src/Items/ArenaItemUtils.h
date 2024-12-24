#ifndef ARENA_ITEM_UTILS_H
#define ARENA_ITEM_UTILS_H

#include <algorithm>

namespace ArenaItemUtils
{
	// Converts Arena weight units to kilograms.
	constexpr double KilogramsDivisor = 256.0;

	constexpr int FistsWeaponID = -1;
	constexpr int RangedWeaponIDs[] = { 16, 17 };

	bool isRangedWeapon(int weaponID);
}

#endif
