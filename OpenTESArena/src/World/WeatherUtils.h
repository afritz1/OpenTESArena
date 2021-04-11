#ifndef WEATHER_UTILS_H
#define WEATHER_UTILS_H

#include "../Assets/ArenaTypes.h"

class WeatherDefinition;

namespace WeatherUtils
{
	// Filters the weather for a location (i.e. if it's attempting to have snow in a desert).
	WeatherDefinition getFilteredWeather(const WeatherDefinition &weatherDef, ArenaTypes::ClimateType climateType);

	// Convenience function until more things are using WeatherDefinition.
	ArenaTypes::WeatherType getLegacyWeather(const WeatherDefinition &weatherDef);
}

#endif
