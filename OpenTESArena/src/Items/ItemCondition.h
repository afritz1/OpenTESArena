#ifndef ITEM_CONDITION_H
#define ITEM_CONDITION_H

#include <string>

// The specifics for "item condition" aren't in the manual, so I think they should
// be redesigned for this project.

// The only items with a condition are weapons, armor, and shields. It says that
// potions are "fragile", but that doesn't really have any gameplay effect at all.

class ArmorMaterial;
class Metal;

enum class BodyPartName;
enum class ItemConditionName;
enum class ShieldType;
enum class WeaponType;

class ItemCondition
{
private:
	int currentCondition, maxCondition, degradeRate;
public:
	// Item condition for a piece of armor of the given type.
	ItemCondition(BodyPartName partName, const ArmorMaterial &material);
	
	// Item condition for a shield of the given type.
	ItemCondition(ShieldType shieldType, const Metal &metal);

	// Item condition for a weapon of the given type, excluding fists.
	ItemCondition(WeaponType weaponType, const Metal &metal);

	// Item condition for fists. Fists do not degrade in condition.
	ItemCondition();
	~ItemCondition();
	
	ItemConditionName getCurrentConditionName() const;
	bool isBroken() const;

	// Set the condition back to full.
	void repairFully();

	// Replenish a small amount of condition. This is exclusively for Knights (or 
	// any class with active item repairing).
	void repairSlightly();

	// Call this whenever the item is being used in a way that degrades it, like 
	// when a weapon hits something, or when armor is hit.
	void degrade();
};

#endif
