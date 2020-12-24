#ifndef MAP_TYPE_H
#define MAP_TYPE_H

// A unique identifier for each kind of game space the player is in. Each type has certain
// data modifiers, such as how to handle level generation outside of the defined area.

enum class MapType
{
	Interior,
	City,
	Wilderness
};

#endif
