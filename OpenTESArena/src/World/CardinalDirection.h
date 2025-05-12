#ifndef CARDINAL_DIRECTION_H
#define CARDINAL_DIRECTION_H

#include "CardinalDirectionName.h"
#include "Coord.h"
#include "../Math/Vector2.h"

// North, northeast, southwest, etc..
namespace CardinalDirection
{
	// Cardinal directions in the XZ plane (bird's eye view).
	// CANNOT INITIALIZE FROM VOXELUTILS due to global initialization order uncertainty.
	constexpr WorldDouble2 North(-1.0, 0.0);
	constexpr WorldDouble2 South(1.0, 0.0);
	constexpr WorldDouble2 East(0.0, -1.0);
	constexpr WorldDouble2 West(0.0, 1.0);

	constexpr std::pair<CardinalDirectionName, const char*> DisplayNames[] =
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

	CardinalDirectionName getDirectionName(const WorldDouble2 &direction);
}

#endif
