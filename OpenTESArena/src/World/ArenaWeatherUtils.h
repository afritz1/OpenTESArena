#ifndef ARENA_WEATHER_UTILS_H
#define ARENA_WEATHER_UTILS_H

#include "../Assets/ArenaTypes.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/utilities/Buffer.h"

class Color;
class ExeData;
class Random;
class TextureManager;

struct TextureAssetReference;

// Various functions for working with the original game's weather.

namespace ArenaWeatherUtils
{
	constexpr int RAINDROP_FAST_COUNT = 20;
	constexpr int RAINDROP_MEDIUM_COUNT = 20;
	constexpr int RAINDROP_SLOW_COUNT = 20;
	constexpr int RAINDROP_TOTAL_COUNT = RAINDROP_FAST_COUNT + RAINDROP_MEDIUM_COUNT + RAINDROP_SLOW_COUNT;

	constexpr int RAINDROP_FAST_PIXELS_PER_FRAME_Y =
		static_cast<int>((3.0 / 25.0) * static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT));
	constexpr int RAINDROP_MEDIUM_PIXELS_PER_FRAME_Y =
		static_cast<int>((1.0 / 10.0) * static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT));
	constexpr int RAINDROP_SLOW_PIXELS_PER_FRAME_Y =
		static_cast<int>((2.0 / 25.0) * static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT));

	constexpr int RAINDROP_FAST_PIXELS_PER_FRAME_X = -RAINDROP_FAST_PIXELS_PER_FRAME_Y / 2;
	constexpr int RAINDROP_MEDIUM_PIXELS_PER_FRAME_X = -RAINDROP_MEDIUM_PIXELS_PER_FRAME_Y / 2;
	constexpr int RAINDROP_SLOW_PIXELS_PER_FRAME_X = -RAINDROP_SLOW_PIXELS_PER_FRAME_Y / 2;

	constexpr double THUNDERSTORM_SKY_FLASH_SECONDS = 0.35;
	constexpr double THUNDERSTORM_BOLT_SECONDS = 0.15;

	constexpr int SNOWFLAKE_FAST_COUNT = 20;
	constexpr int SNOWFLAKE_MEDIUM_COUNT = 30;
	constexpr int SNOWFLAKE_SLOW_COUNT = 50;
	constexpr int SNOWFLAKE_TOTAL_COUNT = SNOWFLAKE_FAST_COUNT + SNOWFLAKE_MEDIUM_COUNT + SNOWFLAKE_SLOW_COUNT;

	constexpr int SNOWFLAKE_FAST_PIXELS_PER_FRAME_Y =
		static_cast<int>((1.0 / 25.0) * static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT));
	constexpr int SNOWFLAKE_MEDIUM_PIXELS_PER_FRAME_Y =
		static_cast<int>((3.0 / 100.0) * static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT));
	constexpr int SNOWFLAKE_SLOW_PIXELS_PER_FRAME_Y =
		static_cast<int>((1.0 / 50.0) * static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT));

	constexpr int SNOWFLAKE_PIXELS_PER_FRAME_X = 2; // Either left or right.

	constexpr int SNOWFLAKE_FAST_SIZE = 2;
	constexpr int SNOWFLAKE_MEDIUM_SIZE = 2;
	constexpr int SNOWFLAKE_SLOW_SIZE = 1;

	constexpr double SNOWFLAKE_MIN_SECONDS_BEFORE_DIRECTION_CHANGE =
		1.0 / static_cast<double>(ArenaRenderUtils::FRAMES_PER_SECOND);

	// Helper functions for determining what category a weather type falls into.
	bool isClear(ArenaTypes::WeatherType weatherType);
	bool isOvercast(ArenaTypes::WeatherType weatherType);
	bool isRain(ArenaTypes::WeatherType weatherType);
	bool isSnow(ArenaTypes::WeatherType weatherType);

	// Returns whether existing fog on a given day in a month is extra heavy.
	bool fogIsHeavy(int currentDay);

	// Returns whether rainy weather is also a thunderstorm.
	bool rainIsThunderstorm(Random &random);

	// Whether an individual snowflake should randomly switch between left/right at this point in time
	// (not sure how frequently this is checked).
	bool shouldSnowflakeChangeDirection(Random &random);

	// Returns a filtered version of the given weather so that, i.e., deserts can't have snow.
	ArenaTypes::WeatherType getFilteredWeatherType(ArenaTypes::WeatherType weatherType,
		ArenaTypes::ClimateType climateType);

	// Gets the fog distance associated with the given weather type.
	double getFogDistanceFromWeather(ArenaTypes::WeatherType weatherType);

	// Creates a sky color buffer from the given weather. This covers the entire day and night.
	Buffer<Color> makeSkyColors(ArenaTypes::WeatherType weatherType, TextureManager &textureManager);

	// Creates a color buffer for thunderstorm flashes in the sky.
	Buffer<uint8_t> makeThunderstormColors(const ExeData &exeData);

	// Creates a buffer of texture asset references for lightning bolts.
	Buffer<Buffer<TextureAssetReference>> makeLightningBoltTextureAssetRefs(TextureManager &textureManager);
}

#endif
