#ifndef ACCESSORY_TYPE_H
#define ACCESSORY_TYPE_H

// A unique identifier for each kind of potentially metal accessory. Non-metal
// accessories like marks and crystals are called "trinkets" instead.

// There should be a mapping somewhere of AccessoryType to integer, for the max
// allowed number of equipped accessories for each kind.

enum class AccessoryType
{
	Amulet,
	Belt,
	Bracelet,
	Bracers,
	Ring,
	Torc
};

#endif