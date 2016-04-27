#ifndef MISCELLANEOUS_ITEM_TYPE_H
#define MISCELLANEOUS_ITEM_TYPE_H

// A unique identifier for each kind of miscellaneous item.

// Followers were treated as inventory items in the original game, but that seems
// a bit strange. The remake should treat them as entities, not items.

enum class MiscellaneousItemType
{
	// Books (these aren't readable. Just dummy types. For them to actually be
	// readable, a book would need to be a derived class of MiscellaneousItem).
	Book,

	// Keys (not equipable. The inventory should search to see if a key is contained).
	Key,

	// Empty potion flask? Where does it go when the potion is consumed?! *crunch*

	// Main quest.
	StaffPiece, // StaffHead, StaffTail...?

	// Lights.
	Torch,

	// Unknown (just as a placeholder).
	Unknown
};

#endif
