#ifndef WEAPON_TYPE_H
#define WEAPON_TYPE_H

// A unique identifier for each kind of weapon. The weapon type is used to determine 
// the graphics for the weapon, base statistics, and if a character can use it.
enum class WeaponType
{
	BattleAxe,
	Broadsword,
	Claymore,
	Dagger,
	DaiKatana,
	Fists, // No chain or plate animation used in the original.
	Flail,
	Katana,
	LongBow,
	Longsword,
	Mace,
	Saber,
	ShortBow,
	Shortsword,
	Staff,
	Tanto,
	Wakizashi,
	WarAxe,
	Warhammer
};

#endif