#ifndef ARENA_ITEM_UTILS_H
#define ARENA_ITEM_UTILS_H

class Random;

enum class ArmorMaterialType;

namespace ArenaItemUtils
{
	// Converts Arena weight units to kilograms.
	constexpr double KilogramsDivisor = 256.0;

	constexpr int FistsWeaponID = -1;
	constexpr int RangedWeaponIDs[] = { 16, 17 };

	constexpr int DoorKeyCount = 12;
	constexpr int InvalidDoorKeyID = -1;

	bool isFistsWeapon(int weaponID);
	bool isRangedWeapon(int weaponID);

	ArmorMaterialType getRandomArmorMaterialType(Random &random);
}

#endif
