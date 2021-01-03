#ifndef ARENA_RENDER_UTILS_H
#define ARENA_RENDER_UTILS_H

#include <cstdint>

namespace ArenaRenderUtils
{
	constexpr int SCREEN_WIDTH = 320;
	constexpr int SCREEN_HEIGHT = 200;
	constexpr int BITS_PER_PIXEL = 8;

	// Hardcoded palette indices with special behavior in the original game's renderer.
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_LOWEST = 1;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_HIGHEST = 13;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_DIVISOR = 14;
	constexpr uint8_t PALETTE_INDEX_SKY_LEVEL_LOWEST = 1;
	constexpr uint8_t PALETTE_INDEX_SKY_LEVEL_HIGHEST = 13;
	constexpr uint8_t PALETTE_INDEX_SKY_LEVEL_DIVISOR = 14;
	constexpr uint8_t PALETTE_INDEX_RED_SRC1 = 14;
	constexpr uint8_t PALETTE_INDEX_RED_SRC2 = 15;
	constexpr uint8_t PALETTE_INDEX_RED_DST1 = 158;
	constexpr uint8_t PALETTE_INDEX_RED_DST2 = 159;
	constexpr uint8_t PALETTE_INDEX_NIGHT_LIGHT = 113;
	constexpr uint8_t PALETTE_INDEX_NIGHT_LIGHT_ACTIVE = 97;
	constexpr uint8_t PALETTE_INDEX_NIGHT_LIGHT_INACTIVE = 112;
	constexpr uint8_t PALETTE_INDEX_PUDDLE_EVEN_ROW = 30;
	constexpr uint8_t PALETTE_INDEX_PUDDLE_ODD_ROW = 103;

	// Various functions for determining the type of palette index.
	bool IsGhostTexel(uint8_t texel);
	bool IsPuddleTexel(uint8_t texel);
	bool IsCloudTexel(uint8_t texel);
}

#endif
