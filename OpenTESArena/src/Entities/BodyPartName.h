#ifndef BODY_PART_NAME_H
#define BODY_PART_NAME_H

// A unique identifier for each body part that can have an equipped piece of armor.
// The accessory and trinket slots are given by the AccessoryType and TrinketName,
// respectively. The held weapon and shield will just be in their own unnamed slots
// because they are not affected by armor ratings.

// I decided to elide the "waist" body part, since belts are considered an accessory,
// so there would never be any equipment in the "waist" slot, and the BodyPart type
// is intended for armor pieces, not accessories. Some shields protect the waist, so 
// I think I'll just roll that in with the chest or something.

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
