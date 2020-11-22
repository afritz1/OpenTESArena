#ifndef INTERIOR_TYPE_H
#define INTERIOR_TYPE_H

// Helps determine entity definitions during level generation.

// @todo: add an InteriorDefinition at some point to further de-hardcode things. It would contain
// rulerIsMale, etc.. Might also have a variable for loot piles when sneaking in at night.

enum class InteriorType
{
	Crypt, // WCRYPT
	Dungeon, // DUNGEON
	Equipment, // EQUIP
	House, // BS
	MagesGuild, // MAGE
	Noble, // NOBLE
	Palace, // PALACE
	Tavern, // TAVERN
	Temple, // TEMPLE
	Tower // TOWER
};

#endif
