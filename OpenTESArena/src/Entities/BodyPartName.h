#ifndef BODY_PART_NAME_H
#define BODY_PART_NAME_H

// A unique identifier for each body part that can have an equipped piece of armor.
// The accessory and trinket slots are given by the AccessoryType and TrinketType,
// respectively. The held weapon and shield will just be in their own unnamed slots
// because they are not affected by armor ratings.
enum class BodyPartName
{
	Head,
	LeftShoulder,
	RightShoulder,
	Chest,
	Hands,
	Legs,
	Feet
};

#endif
