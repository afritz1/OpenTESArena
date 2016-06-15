#ifndef ARMOR_PIECE_TYPE_H
#define ARMOR_PIECE_TYPE_H

// A unique identifier for each type of armor piece. There is not a one-to-one
// mapping with BodyPartNames because a shield does not correspond to a body
// part (with this design, weapon hands are separate from armor hands).
enum class ArmorType
{
	Helm,
	LeftPauldron,
	RightPauldron,
	Cuirass,
	Gauntlets,
	Shield,
	Greaves,
	Boots
};

#endif
