#pragma once

#include <iterator>

#include "../Assets/ArenaTypes.h"

class Random;

struct ExeData;

namespace ArenaItemUtils
{
	// Converts Arena weight units to kilograms.
	constexpr double KilogramsDivisor = 256.0;

	constexpr int FistsWeaponID = -1;
	constexpr int MeleeWeaponIDs[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	constexpr int RangedWeaponIDs[] = { 16, 17 };
	constexpr int MeleeWeaponCount = static_cast<int>(std::size(ArenaItemUtils::MeleeWeaponIDs));
	constexpr int RangedWeaponCount = static_cast<int>(std::size(ArenaItemUtils::RangedWeaponIDs));

	constexpr int DoorKeyCount = 12;
	constexpr int InvalidDoorKeyID = -1;

	bool isFistsWeapon(int weaponID);
	bool isMeleeWeapon(int weaponID);
	bool isRangedWeapon(int weaponID);

	int getArmorClassMagicItemBasePrice(int materialID, const ExeData &exeData);
	int getAttributeEnhancementMagicItemBasePrice(int baseItemID, int attributeID, const ExeData &exeData);

	ArenaArmorMaterialType getRandomArmorMaterialType(Random &random);
}
