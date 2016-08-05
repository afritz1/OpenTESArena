#ifndef MISCELLANEOUS_ITEM_TYPE_H
#define MISCELLANEOUS_ITEM_TYPE_H

// A unique identifier for each kind of miscellaneous item.

// This enumeration might need some refining.

// Followers were treated as inventory items in the original game, but that seems
// a bit strange. The remake should treat them as entities, not items.

enum class MiscellaneousItemType
{
	// Books (these aren't readable. Just dummy types. For them to actually be
	// readable, a book would need to be a derived class of MiscellaneousItem).
	Book,

	// Keys (not equipable. The inventory should search to see if a key is contained).
	Key,

	// Main quest. I don't think there should be multiple types for inventory pieces,
	// but the in-game sprite should be differentiable somehow, regardless. The
	// number of pieces held is all that really matters quest-wise.
	StaffPiece, // StaffHead, StaffTail...?

	// Lights.
	Torch,

	// Unknown (just as a placeholder).
	Unknown
};

#endif
