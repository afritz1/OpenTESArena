#ifndef WORLD_TYPE_H
#define WORLD_TYPE_H

// A unique identifier for each kind of place the player is in. Each world type has certain
// data modifiers, such as how to handle level generation outside of the defined area.

enum class WorldType
{
	Interior,
	City,
	Wilderness
};

#endif
