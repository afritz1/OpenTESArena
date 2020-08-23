#include <unordered_map>

#include "CardinalDirection.h"
#include "CardinalDirectionName.h"

#include "components/debug/Debug.h"

// Wikipedia says the intermediate directions don't have a space, so that's the
// convention I'll use here.
const std::unordered_map<CardinalDirectionName, std::string> CardinalDirectionDisplayNames =
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

CardinalDirectionName CardinalDirection::getDirectionName(const NewDouble2 &direction)
{
	const NewDouble2 northEast = CardinalDirection::North.slerp(CardinalDirection::East, 0.5);
	const NewDouble2 southEast = CardinalDirection::South.slerp(CardinalDirection::East, 0.5);
	const NewDouble2 southWest = CardinalDirection::South.slerp(CardinalDirection::West, 0.5);
	const NewDouble2 northWest = CardinalDirection::North.slerp(CardinalDirection::West, 0.5);

	// Each direction gets an equal slice of the circle's area.
	// (I'm not sure why the deviation is 1/12th; at a glance it should be 1/8th).
	const double deviation = 1.0 / 12.0;
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
		DebugCrash("Invalid CardinalDirection (" + std::to_string(direction.x) + ", " +
			std::to_string(direction.y) + ").");
	}

	return name;
}

const std::string &CardinalDirection::toString(CardinalDirectionName directionName)
{
	const std::string &displayName = CardinalDirectionDisplayNames.at(directionName);
	return displayName;
}
