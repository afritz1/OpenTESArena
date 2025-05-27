#ifndef CHARACTER_CLASS_DEFINITION_H
#define CHARACTER_CLASS_DEFINITION_H

#include <vector>

#include "components/utilities/BufferView.h"

using CharacterClassCategoryID = int; // Warrior/mage/thief

struct CharacterClassDefinition
{
	char name[64];
	CharacterClassCategoryID categoryID;
	char preferredAttributes[128]; // Description in character creation.
	std::vector<int> allowedArmors; // 0 = leather, 1 = chain, etc..
	std::vector<int> allowedShields; // 0 = buckler, 1 = round shield, etc..
	std::vector<int> allowedWeapons; // 0 = staff, 1 = sword, etc..
	bool castsMagic;
	int healthDie; // d8, d20, etc..
	double spellPointsMultiplier;
	int initialExpCap;
	double lockpickPercent; // Lockpick effectiveness percent.
	bool criticalHit;
	int originalClassIndex; // Non-negative if derived from original game.

	CharacterClassDefinition();

	void init(const char *name, CharacterClassCategoryID categoryID, const char *preferredAttributes,
		BufferView<const int> allowedArmors, BufferView<const int> allowedShields, BufferView<const int> allowedWeapons,
		bool castsMagic, int healthDie, double spellPointsMultiplier, int initialExpCap, double lockpickPercent,
		bool criticalHit, int originalClassIndex);

	int getAllowedArmorCount() const;
	int getAllowedShieldCount() const;
	int getAllowedWeaponCount() const;
	int getAllowedArmor(int index) const;
	int getAllowedShield(int index) const;
	int getAllowedWeapon(int index) const;
	int getExperienceCap(int level) const;
};

#endif
