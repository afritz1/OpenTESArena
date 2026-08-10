#pragma once

#include <iterator>

#include "../Assets/ArenaTypes.h"

class Random;

struct ExeData;

namespace ArenaItemUtils
{
	// Converts Arena weight units to kilograms.
	constexpr double KilogramsDivisor = 256.0;

	constexpr ArenaWeaponTypeID FistsWeaponID = -1;
	constexpr ArenaWeaponTypeID MeleeWeaponIDs[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	constexpr ArenaWeaponTypeID RangedWeaponIDs[] = { 16, 17 };
	constexpr int MeleeWeaponCount = static_cast<int>(std::size(ArenaItemUtils::MeleeWeaponIDs));
	constexpr int RangedWeaponCount = static_cast<int>(std::size(ArenaItemUtils::RangedWeaponIDs));

	constexpr ArenaArmorTypeID ChestArmorTypeID = 0;
	constexpr ArenaArmorTypeID HandsArmorTypeID = 1;
	constexpr ArenaArmorTypeID LegsArmorTypeID = 2;
	constexpr ArenaArmorTypeID LeftShoulderArmorTypeID = 3;
	constexpr ArenaArmorTypeID RightShoulderArmorTypeID = 4;
	constexpr ArenaArmorTypeID HeadArmorTypeID = 5;
	constexpr ArenaArmorTypeID FeetArmorTypeID = 6;
	constexpr ArenaArmorTypeID ShieldArmorTypeIDs[] = { 7, 8, 9, 10 };

	constexpr int DoorKeyCount = 12;
	constexpr int InvalidDoorKeyID = -1;

	bool isFistsWeapon(ArenaWeaponTypeID weaponID);
	bool isMeleeWeapon(ArenaWeaponTypeID weaponID);
	bool isRangedWeapon(ArenaWeaponTypeID weaponID);

	int getArmorClassMagicItemBasePrice(int materialID, const ExeData &exeData);
	int getAttributeEnhancementMagicItemBasePrice(int baseItemID, int attributeID, const ExeData &exeData);

	ArenaArmorMaterialType getRandomArmorMaterialType(Random &random);
}
