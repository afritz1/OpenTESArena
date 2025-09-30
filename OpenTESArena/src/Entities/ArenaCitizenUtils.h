#ifndef ARENA_CITIZEN_UTILS_H
#define ARENA_CITIZEN_UTILS_H

#include <cstdint>

#include "../Assets/MIFUtils.h"
#include "../Rendering/ArenaRenderUtils.h"

namespace ArenaCitizenUtils
{
	constexpr int CITY_COUNT = 15;
	constexpr int WILD_COUNT = 7;

	// Min and max spawn distance from player.
	constexpr int SPAWN_DISTANCE_MIN_UNITS = 2048;
	constexpr int SPAWN_DISTANCE_MAX_UNITS = 3072;
	constexpr double SPAWN_DISTANCE_MIN_REAL = static_cast<double>(SPAWN_DISTANCE_MIN_UNITS) / MIFUtils::ARENA_UNITS;
	constexpr double SPAWN_DISTANCE_MAX_REAL = static_cast<double>(SPAWN_DISTANCE_MAX_UNITS) / MIFUtils::ARENA_UNITS;

	// How far away a citizen will consider idling to the player.
	constexpr int IDLE_DISTANCE_UNITS = 200;
	constexpr double IDLE_DISTANCE_REAL = static_cast<double>(IDLE_DISTANCE_UNITS) / MIFUtils::ARENA_UNITS;
	constexpr double IDLE_DISTANCE_REAL_SQR = IDLE_DISTANCE_REAL * IDLE_DISTANCE_REAL;

	constexpr int MOVE_UNITS_PER_FRAME = static_cast<int>(MIFUtils::ARENA_UNITS / 10.0);
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

	constexpr int DEATH_MIN_GOLD_PIECES = 1;
	constexpr int DEATH_MAX_GOLD_PIECES = 4;
}

#endif
