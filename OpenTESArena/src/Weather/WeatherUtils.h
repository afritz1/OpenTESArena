#pragma once

#include "../Assets/ArenaTypes.h"

struct WeatherDefinition;

namespace WeatherUtils
{
	// Filters the weather for a location (i.e. if it's attempting to have snow in a desert).
	WeatherDefinition getFilteredWeather(const WeatherDefinition &weatherDef, ArenaClimateType climateType);
}
