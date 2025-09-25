#ifndef ARENA_RENDER_UTILS_H
#define ARENA_RENDER_UTILS_H

#include <cstdint>
#include <string>

#include "../Assets/MIFUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Span2D.h"

class Random;
class TextureManager;

enum class MapType;

struct Clock;
struct ExeDataWeather;

struct ArenaFogState
{
	Buffer<uint16_t> fogTxt;
	Buffer<uint8_t> fogLgt;

	short PlayerX;
	short PlayerY;
	short PlayerAngle;
	short WORD_4b80_191b; // Initially 0, it will be 4 by the first call of this function, and +4 every time after (every game update).
	short WORD_4b80_191d; // Initially 0, it will be 4 by the first call of this function, and +4 every time after (every game update).

	double currentSeconds;

	ArenaFogState();

	void init(TextureManager &textureManager);

	void update(double dt);
};

namespace ArenaRenderUtils
{
	constexpr int SCREEN_WIDTH = 320;
	constexpr int SCREEN_HEIGHT = 200;
	constexpr int SCENE_UI_HEIGHT = 53;
	constexpr int SCENE_VIEW_HEIGHT = SCREEN_HEIGHT - SCENE_UI_HEIGHT;
	constexpr double SCREEN_WIDTH_REAL = static_cast<double>(SCREEN_WIDTH);
	constexpr double SCREEN_HEIGHT_REAL = static_cast<double>(SCREEN_HEIGHT);
	constexpr double ASPECT_RATIO = SCREEN_WIDTH_REAL / SCREEN_HEIGHT_REAL;
	constexpr double PRESENTED_ASPECT_RATIO = 4.0 / 3.0;
	constexpr int BITS_PER_PIXEL = 8;

	// Height multiplier for pixels due to the difference between how the game renders internally and how it appeared
	// on players' monitors.
	constexpr double TALL_PIXEL_RATIO = ASPECT_RATIO / PRESENTED_ASPECT_RATIO;

	constexpr int FRAMES_PER_SECOND = 25;

	// Texture limits.
	constexpr int DEFAULT_VOXEL_TEXTURE_COUNT = 64;
	constexpr int DEFAULT_FLAT_TEXTURE_COUNT = 256;

	// Amount of a sliding/raising door that is visible when fully open.
	constexpr double DOOR_MIN_VISIBLE = 0.10;

	// Hardcoded palette indices with special behavior in the original game's renderer.
	constexpr uint8_t PALETTE_INDEX_TRANSPARENT = 0;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_LOWEST = 1;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_HIGHEST = 13;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_DIVISOR = 14;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_SRC1 = 14;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_SRC2 = 15;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_DST1 = 158;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_DST2 = 159;
	constexpr uint8_t PALETTE_INDEX_WINDOW = 113;
	constexpr uint8_t PALETTE_INDEX_PUDDLE_EVEN_ROW = 30;
	constexpr uint8_t PALETTE_INDEX_PUDDLE_ODD_ROW = 103;
	constexpr uint8_t PALETTE_INDEX_DRY_CHASM_COLOR = 112;
	constexpr uint8_t PALETTE_INDEX_RAINDROP = 103;
	constexpr uint8_t PALETTE_INDEX_SNOWFLAKE = 16;
	constexpr uint8_t PALETTE_INDICES_SKY_COLOR[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	constexpr uint8_t PALETTE_INDEX_SKY_COLOR_FOG = 100;
	// Thunderstorm sky colors and window colors come from the executable.

	constexpr int RAINDROP_TEXTURE_WIDTH = 3;
	constexpr int RAINDROP_TEXTURE_HEIGHT = 8;
	constexpr int SNOWFLAKE_FAST_TEXTURE_WIDTH = 2;
	constexpr int SNOWFLAKE_FAST_TEXTURE_HEIGHT = 2;
	constexpr int SNOWFLAKE_MEDIUM_TEXTURE_WIDTH = 2;
	constexpr int SNOWFLAKE_MEDIUM_TEXTURE_HEIGHT = 2;
	constexpr int SNOWFLAKE_SLOW_TEXTURE_WIDTH = 1;
	constexpr int SNOWFLAKE_SLOW_TEXTURE_HEIGHT = 1;

	constexpr int SNOWFLAKE_TEXTURE_WIDTHS[] =
	{
		ArenaRenderUtils::SNOWFLAKE_FAST_TEXTURE_WIDTH,
		ArenaRenderUtils::SNOWFLAKE_MEDIUM_TEXTURE_WIDTH,
		ArenaRenderUtils::SNOWFLAKE_SLOW_TEXTURE_WIDTH
	};

	constexpr int SNOWFLAKE_TEXTURE_HEIGHTS[] =
	{
		ArenaRenderUtils::SNOWFLAKE_FAST_TEXTURE_HEIGHT,
		ArenaRenderUtils::SNOWFLAKE_MEDIUM_TEXTURE_HEIGHT,
		ArenaRenderUtils::SNOWFLAKE_SLOW_TEXTURE_HEIGHT
	};

	constexpr uint8_t RAINDROP_TEXELS[RAINDROP_TEXTURE_WIDTH * RAINDROP_TEXTURE_HEIGHT] =
	{
		0, 0, PALETTE_INDEX_RAINDROP,
		0, 0, PALETTE_INDEX_RAINDROP,
		0, PALETTE_INDEX_RAINDROP, 0,
		0, PALETTE_INDEX_RAINDROP, 0,
		0, PALETTE_INDEX_RAINDROP, 0,
		PALETTE_INDEX_RAINDROP, 0, 0,
		PALETTE_INDEX_RAINDROP, 0, 0,
		PALETTE_INDEX_RAINDROP, 0, 0
	};

	constexpr uint8_t SNOWFLAKE_FAST_TEXELS[SNOWFLAKE_FAST_TEXTURE_WIDTH * SNOWFLAKE_FAST_TEXTURE_HEIGHT] =
	{
		PALETTE_INDEX_SNOWFLAKE, PALETTE_INDEX_SNOWFLAKE,
		PALETTE_INDEX_SNOWFLAKE, PALETTE_INDEX_SNOWFLAKE
	};

	constexpr uint8_t SNOWFLAKE_MEDIUM_TEXELS[SNOWFLAKE_MEDIUM_TEXTURE_WIDTH * SNOWFLAKE_MEDIUM_TEXTURE_HEIGHT] =
	{
		PALETTE_INDEX_SNOWFLAKE, PALETTE_INDEX_SNOWFLAKE,
		PALETTE_INDEX_SNOWFLAKE, PALETTE_INDEX_SNOWFLAKE
	};

	constexpr uint8_t SNOWFLAKE_SLOW_TEXELS[SNOWFLAKE_SLOW_TEXTURE_WIDTH * SNOWFLAKE_SLOW_TEXTURE_HEIGHT] =
	{
		PALETTE_INDEX_SNOWFLAKE
	};

	static const uint8_t *SNOWFLAKE_TEXELS_PTRS[] =
	{
		SNOWFLAKE_FAST_TEXELS,
		SNOWFLAKE_MEDIUM_TEXELS,
		SNOWFLAKE_SLOW_TEXELS
	};

	constexpr double PLAYER_LIGHT_START_RADIUS = 150.0 / MIFUtils::ARENA_UNITS;
	constexpr double PLAYER_LIGHT_END_RADIUS = 320.0 / MIFUtils::ARENA_UNITS;
	constexpr double PLAYER_FOG_LIGHT_START_RADIUS = 250.0 / MIFUtils::ARENA_UNITS;
	constexpr double PLAYER_FOG_LIGHT_END_RADIUS = 575.0 / MIFUtils::ARENA_UNITS;
	constexpr double STREETLIGHT_LIGHT_RADIUS = 3.0 * (MIFUtils::ARENA_UNITS / 100.0);

	const std::string CHASM_WATER_FILENAME = "WATERANI.RCI";
	const std::string CHASM_LAVA_FILENAME = "LAVAANI.RCI";

	// Gets the current ambient light percent for the scene.
	double getAmbientPercent(const Clock &clock, MapType mapType, bool isFoggy);

	// Gets the ambient percent applied to distant objects.
	double getDistantAmbientPercent(const Clock &clock);

	// Used with ghosts and some sky objects.
	bool isLightLevelTexel(uint8_t texel);

	bool isPuddleTexel(uint8_t texel);

	// Draws to an 8-bit 320x200 paletted texture.
	void populateFogTexture(const ArenaFogState &fogState, Span2D<uint8_t> outPixels);
}

#endif
