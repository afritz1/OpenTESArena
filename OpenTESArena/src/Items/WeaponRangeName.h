#ifndef WEAPON_RANGE_NAME_H
#define WEAPON_RANGE_NAME_H

// This enum defines the category of a weapon's range. Bows are ranged;
// everything else is melee. There's no special "staff" weapon that shoots
// magic bolts or anything, though that would be pretty neat.
enum class WeaponRangeName
{
	Melee,
	Ranged
};

#endif