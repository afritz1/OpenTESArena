#ifndef WEATHER_UTILS_H
#define WEATHER_UTILS_H

#include <cstdint>

#include "components/utilities/Buffer.h"

class TextureManager;

enum class ClimateType;
enum class WeatherType;

// Various functions for working with the original game's weather.

namespace WeatherUtils
{
	constexpr double DEFAULT_INTERIOR_FOG_DIST = 25.0; // Arbitrary.

	// Helper functions for determining what category a weather type falls into.
	bool isClear(WeatherType weatherType);
	bool isOvercast(WeatherType weatherType);
	bool isRain(WeatherType weatherType);
	bool isSnow(WeatherType weatherType);

	// Returns a filtered version of the given weather so that, i.e., deserts can't have snow.
	WeatherType getFilteredWeatherType(WeatherType weatherType, ClimateType climateType);

	// Gets the fog distance associated with the given weather type.
	double getFogDistanceFromWeather(WeatherType weatherType);

	// Creates a sky palette from the given weather. This palette covers the entire day
	// (including night colors).
	Buffer<uint32_t> makeExteriorSkyPalette(WeatherType weatherType, TextureManager &textureManager);
}

#endif
