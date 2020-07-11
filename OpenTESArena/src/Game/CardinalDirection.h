#ifndef CARDINAL_DIRECTION_H
#define CARDINAL_DIRECTION_H

#include <string>

#include "../Math/Vector2.h"
#include "../World/VoxelUtils.h"

// North, northeast, southwest, etc..

enum class CardinalDirectionName;

namespace CardinalDirection
{
	// Cardinal directions in the XZ plane (bird's eye view).
	const NewDouble2 North(static_cast<double>(VoxelUtils::North.x), static_cast<double>(VoxelUtils::North.y));
	const NewDouble2 South(static_cast<double>(VoxelUtils::South.x), static_cast<double>(VoxelUtils::South.y));
	const NewDouble2 East(static_cast<double>(VoxelUtils::East.x), static_cast<double>(VoxelUtils::East.y));
	const NewDouble2 West(static_cast<double>(VoxelUtils::West.x), static_cast<double>(VoxelUtils::West.y));

	CardinalDirectionName getDirectionName(const NewDouble2 &direction);
	const std::string &toString(CardinalDirectionName directionName);
}

#endif
