#include "CardinalDirection.h"
#include "CardinalDirectionName.h"

#include "components/debug/Debug.h"

CardinalDirectionName CardinalDirection::getDirectionName(const WorldDouble2 &direction)
{
	// @todo make these constexpr in .h file with sqrt(2)/2 hardcoded probably
	const WorldDouble2 northEast = CardinalDirection::North.slerp(CardinalDirection::East, 0.5);
	const WorldDouble2 southEast = CardinalDirection::South.slerp(CardinalDirection::East, 0.5);
	const WorldDouble2 southWest = CardinalDirection::South.slerp(CardinalDirection::West, 0.5);
	const WorldDouble2 northWest = CardinalDirection::North.slerp(CardinalDirection::West, 0.5);

	// Each direction gets an equal slice of the circle's area.
	// (I'm not sure why the deviation is 1/12th; at a glance it should be 1/8th).
	constexpr double deviation = 1.0 / 12.0;
	auto isCloseEnoughTo = [deviation, &direction](const Double2 &cardinalDirection)
	{
		return direction.dot(cardinalDirection) >= (1.0 - deviation);
	};

	// Find the cardinal direction closest to the given direction. Start with
	// a default name and figure out the true one from there.
	auto name = CardinalDirectionName::North;
	if (isCloseEnoughTo(CardinalDirection::North))
	{
		name = CardinalDirectionName::North;
	}
	else if (isCloseEnoughTo(northEast))
	{
		name = CardinalDirectionName::NorthEast;
	}
	else if (isCloseEnoughTo(CardinalDirection::East))
	{
		name = CardinalDirectionName::East;
	}
	else if (isCloseEnoughTo(southEast))
	{
		name = CardinalDirectionName::SouthEast;
	}
	else if (isCloseEnoughTo(CardinalDirection::South))
	{
		name = CardinalDirectionName::South;
	}
	else if (isCloseEnoughTo(southWest))
	{
		name = CardinalDirectionName::SouthWest;
	}
	else if (isCloseEnoughTo(CardinalDirection::West))
	{
		name = CardinalDirectionName::West;
	}
	else if (isCloseEnoughTo(northWest))
	{
		name = CardinalDirectionName::NorthWest;
	}
	else
	{
		DebugCrash("Invalid CardinalDirection (" + std::to_string(direction.x) + ", " + std::to_string(direction.y) + ").");
	}

	return name;
}
