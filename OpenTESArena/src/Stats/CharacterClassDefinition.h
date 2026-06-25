#pragma once

#include <vector>

#include "../Assets/ArenaTypes.h"

#include "components/utilities/Span.h"

using CharacterClassCategoryID = int; // Warrior/mage/thief

struct CharacterClassDefinition
{
	static constexpr CharacterClassCategoryID CATEGORY_ID_MAGE = 0;
	static constexpr CharacterClassCategoryID CATEGORY_ID_THIEF = 1;
	static constexpr CharacterClassCategoryID CATEGORY_ID_WARRIOR = 2;

	char name[64];
	CharacterClassCategoryID categoryID;
	char categoryName[64]; // Warrior/mage/thief.
	char preferredAttributes[128]; // Description in character creation.
	std::vector<ArenaArmorMaterialType> allowedArmors;
	std::vector<ArenaArmorTypeID> allowedShields; // 7 = buckler, 8 = round shield, etc..
	std::vector<ArenaWeaponTypeID> allowedWeapons; // 0 = staff, 1 = sword, etc..
	bool castsMagic;
	int healthDie; // d8, d20, etc..
	double spellPointsMultiplier;
	int initialExpCap;
	int thievingDivisor;
	bool criticalHit;
	double climbingSpeedScale;
	int originalClassIndex; // Non-negative if derived from original game.

	CharacterClassDefinition();

	void init(const char *name, CharacterClassCategoryID categoryID, const char *categoryName, const char *preferredAttributes,
		Span<const ArenaArmorMaterialType> allowedArmors, Span<const ArenaArmorTypeID> allowedShields, Span<const ArenaWeaponTypeID> allowedWeapons,
		bool castsMagic, int healthDie, double spellPointsMultiplier, int initialExpCap, int thievingDivisor,
		bool criticalHit, double climbingSpeedScale, int originalClassIndex);

	int getAllowedArmorCount() const;
	ArenaArmorMaterialType getAllowedArmor(int index) const;
	bool isArmorMaterialAllowed(ArenaArmorMaterialType materialType) const;

	int getAllowedShieldCount() const;
	ArenaArmorTypeID getAllowedShield(int index) const;
	bool isShieldTypeAllowed(ArenaArmorTypeID typeID) const;

	int getAllowedWeaponCount() const;
	ArenaWeaponTypeID getAllowedWeapon(int index) const;
	bool isWeaponTypeAllowed(ArenaWeaponTypeID typeID) const;

	int getExperienceCap(int level) const;
};
