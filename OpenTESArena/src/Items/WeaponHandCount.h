#ifndef WEAPON_HAND_COUNT_H
#define WEAPON_HAND_COUNT_H

// This could easily be a simple integer, but I want name equivalence and explicit
// range restriction for certain things, so that there is no possibility of it ever 
// being something like zero or three without it being listed as an element here.
enum class WeaponHandCount
{
	One,
	Two
};

#endif
