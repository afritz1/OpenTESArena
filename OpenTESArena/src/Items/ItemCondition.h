#ifndef ITEM_CONDITION_H
#define ITEM_CONDITION_H

#include <string>

// The specifics for "item condition" aren't in the manual, so I think they should
// be redesigned for this project.

// The only items with a condition are weapons, armor, and shields. It says that
// potions are "fragile", but that doesn't really have any gameplay effect at all.

class ArmorMaterial;
class Metal;
class Random;

enum class BodyPartName;
enum class ItemConditionName;
enum class ShieldType;

class ItemCondition
{
private:
	int currentCondition, maxCondition, degradeRate;

	// Initialized by static methods.
	ItemCondition() = default;
public:
	// Item condition for a piece of armor of the given type.
	static ItemCondition makeArmorCondition(BodyPartName partName, const ArmorMaterial &material);

	// Item condition for a shield of the given type.
	static ItemCondition makeShieldCondition(ShieldType shieldType, const Metal &metal);

	// Item condition for a weapon of the given type, excluding fists.
	static ItemCondition makeWeaponCondition(int weaponID, const Metal &metal);

	// Item condition for fists.
	static ItemCondition makeFistsCondition();
	
	ItemConditionName getCurrentConditionName() const;
	bool isBroken() const;

	// Set the condition back to full.
	void repairFully();

	// Replenish a small amount of condition. This is exclusively for Knights (or 
	// any class with active item repairing).
	void repairSlightly(Random &random);

	// Call this whenever the item is being used in a way that degrades it, like 
	// when a weapon hits something, or when armor is hit.
	void degrade();
};

#endif
