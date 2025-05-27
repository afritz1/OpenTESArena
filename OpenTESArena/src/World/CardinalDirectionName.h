#ifndef CARDINAL_DIRECTION_NAME_H
#define CARDINAL_DIRECTION_NAME_H

// Useful for when people give directions. The player's compass is independent of
// any CardinalDirections, though their meanings both equate to the same thing.
enum class CardinalDirectionName
{
	North,
	NorthEast,
	East,
	SouthEast,
	South,
	SouthWest,
	West,
	NorthWest
};

#endif
