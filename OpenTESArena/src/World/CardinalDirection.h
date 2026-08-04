#pragma once

#include "CardinalDirectionName.h"
#include "Coord.h"
#include "../Math/MathUtils.h"
#include "../Math/Vector2.h"

// North, northeast, southwest, etc..
namespace CardinalDirection
{
	// Cardinal directions in the XZ plane (bird's eye view).
	// CANNOT INITIALIZE FROM VOXELUTILS due to global initialization order uncertainty.
	constexpr WorldDouble2 North2D(-1.0, 0.0);
	constexpr WorldDouble2 South2D(1.0, 0.0);
	constexpr WorldDouble2 East2D(0.0, -1.0);
	constexpr WorldDouble2 West2D(0.0, 1.0);

	constexpr WorldDouble3 North3D(-1.0, 0.0, 0.0);
	constexpr WorldDouble3 South3D(1.0, 0.0, 0.0);
	constexpr WorldDouble3 East3D(0.0, 0.0, -1.0);
	constexpr WorldDouble3 West3D(0.0, 0.0, 1.0);

	constexpr Degrees DegreesNorth = 270.0;
	constexpr Degrees DegreesSouth = 90.0;
	constexpr Degrees DegreesEast = 180.0;
	constexpr Degrees DegreesWest = 0.0;

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

	CardinalDirectionName getDirectionName(WorldDouble2 direction);
}
