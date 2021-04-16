#ifndef ARENA_WEATHER_UTILS_H
#define ARENA_WEATHER_UTILS_H

#include <cstdint>

#include "../Assets/ArenaTypes.h"

#include "components/utilities/Buffer.h"

class Random;
class Color;
class TextureManager;

// Various functions for working with the original game's weather.

namespace ArenaWeatherUtils
{
	static constexpr int RAINDROP_COUNT = 64;

	// Raindrop velocities in pixels per second.
	static constexpr int RAINDROP_VELOCITY_X = -200;
	static constexpr int RAINDROP_VELOCITY_Y = 500;

	static constexpr double THUNDERSTORM_FLASH_SECONDS = 0.35; // Duration of sky flash.
	static constexpr double THUNDERSTORM_BOLT_SECONDS = 0.10; // Duration of lightning bolt.

	// Helper functions for determining what category a weather type falls into.
	bool isClear(ArenaTypes::WeatherType weatherType);
	bool isOvercast(ArenaTypes::WeatherType weatherType);
	bool isRain(ArenaTypes::WeatherType weatherType);
	bool isSnow(ArenaTypes::WeatherType weatherType);

	// Returns whether existing fog on a given day in a month is extra heavy.
	bool fogIsHeavy(int currentDay);

	// Returns whether rainy weather is also a thunderstorm.
	bool rainIsThunderstorm(Random &random);

	// Returns a filtered version of the given weather so that, i.e., deserts can't have snow.
	ArenaTypes::WeatherType getFilteredWeatherType(ArenaTypes::WeatherType weatherType,
		ArenaTypes::ClimateType climateType);

	// Gets the fog distance associated with the given weather type.
	double getFogDistanceFromWeather(ArenaTypes::WeatherType weatherType);

	// Creates a sky palette from the given weather. This palette covers the entire day (including night colors).
	Buffer<Color> makeSkyColors(ArenaTypes::WeatherType weatherType, TextureManager &textureManager);
}

#endif
