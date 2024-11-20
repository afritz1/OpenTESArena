#ifndef ARENA_CITIZEN_UTILS_H
#define ARENA_CITIZEN_UTILS_H

#include <cstdint>

#include "../Assets/MIFUtils.h"
#include "../Rendering/ArenaRenderUtils.h"

namespace ArenaCitizenUtils
{
	// How far away a citizen will consider idling to the player.
	constexpr int IDLE_DISTANCE_UNITS = 200;
	constexpr double IDLE_DISTANCE_REAL = static_cast<double>(IDLE_DISTANCE_UNITS) / MIFUtils::ARENA_UNITS;
	constexpr double IDLE_DISTANCE_REAL_SQR = IDLE_DISTANCE_REAL * IDLE_DISTANCE_REAL;

	constexpr int MOVE_UNITS_PER_FRAME = static_cast<int>(MIFUtils::ARENA_UNITS) / 8;
	constexpr int MOVE_UNITS_PER_SECOND = MOVE_UNITS_PER_FRAME * ArenaRenderUtils::FRAMES_PER_SECOND;
	constexpr double MOVE_SPEED_PER_SECOND = static_cast<double>(MOVE_UNITS_PER_SECOND) / MIFUtils::ARENA_UNITS;

	constexpr int8_t DIRECTION_NORTH = 0;
	constexpr int8_t DIRECTION_EAST = 1;
	constexpr int8_t DIRECTION_SOUTH = 2;
	constexpr int8_t DIRECTION_WEST = 3;
	constexpr int8_t DIRECTION_INDICES[] =
	{
		DIRECTION_NORTH,
		DIRECTION_EAST,
		DIRECTION_SOUTH,
		DIRECTION_WEST
	};
}

#endif
