#include <cassert>
#include <exception>
#include <map>

#include "CardinalDirection.h"

#include "CardinalDirectionName.h"

// Wikipedia says the intermediate directions don't have a space, so that's the
// convention I'll use here.
const std::map<CardinalDirectionName, std::string> CardinalDirectionDisplayNames =
{
	{ CardinalDirectionName::North, "North" },
	{ CardinalDirectionName::NorthEast, "Northeast" },
	{ CardinalDirectionName::East, "East" },
	{ CardinalDirectionName::SouthEast, "Southeast" },
	{ CardinalDirectionName::South, "South" },
	{ CardinalDirectionName::SouthWest, "Southwest" },
	{ CardinalDirectionName::West, "West" },
	{ CardinalDirectionName::NorthWest, "Northwest" }
};

CardinalDirection::CardinalDirection(const Float2d &direction)
{
	// The caller should normalize their vector. A "direction" is implied to be normalized.
	assert(direction.isNormalized());

	const Float2d north(0.0, 1.0);
	const Float2d south(0.0, -1.0);
	const Float2d east(1.0, 0.0);
	const Float2d west(-1.0, 0.0);
	const Float2d northEast = north.slerp(east, 0.5);
	const Float2d southEast = south.slerp(east, 0.5);
	const Float2d southWest = south.slerp(west, 0.5);
	const Float2d northWest = north.slerp(west, 0.5);

	// The spherical interpolations should already be normalized if their parent vectors
	// are normalized.
	assert(north.isNormalized());
	assert(south.isNormalized());
	assert(east.isNormalized());
	assert(west.isNormalized());
	assert(northEast.isNormalized());
	assert(southEast.isNormalized());
	assert(southWest.isNormalized());
	assert(northWest.isNormalized());

	// Each direction gets an eighth of the circle's area. In dot product terms,
	// that's an allowance of 0.25 deviation from the direction.
	const double deviation = 0.25;
	auto isCloseEnoughTo = [deviation, &direction](const Float2d &cardinalDirection)
	{
		return direction.dot(cardinalDirection) >= (1.0 - deviation);
	};

	// Find the cardinal direction closest to the given direction. Start with
	// a default name and figure out the true one from there.
	auto name = CardinalDirectionName::North;
	if (isCloseEnoughTo(north))
	{
		name = CardinalDirectionName::North;
	}
	else if (isCloseEnoughTo(northEast))
	{
		name = CardinalDirectionName::NorthEast;
	}
	else if (isCloseEnoughTo(east))
	{
		name = CardinalDirectionName::East;
	}
	else if (isCloseEnoughTo(southEast))
	{
		name = CardinalDirectionName::SouthEast;
	}
	else if (isCloseEnoughTo(south))
	{
		name = CardinalDirectionName::South;
	}
	else if (isCloseEnoughTo(southWest))
	{
		name = CardinalDirectionName::SouthWest;
	}
	else if (isCloseEnoughTo(west))
	{
		name = CardinalDirectionName::West;
	}
	else if (isCloseEnoughTo(northWest))
	{
		name = CardinalDirectionName::NorthWest;
	}
	else
	{
		throw std::runtime_error("Invalid direction for CardinalDirection.");
	}

	this->directionName = name;
}

CardinalDirection::~CardinalDirection()
{

}

CardinalDirectionName CardinalDirection::getDirectionName() const
{
	return this->directionName;
}

std::string CardinalDirection::toString() const
{
	auto displayName = CardinalDirectionDisplayNames.at(this->getDirectionName());
	return displayName;
}
