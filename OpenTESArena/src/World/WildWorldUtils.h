#ifndef WILD_WORLD_UTILS_H
#define WILD_WORLD_UTILS_H

#include <string>

enum class ClimateType;
enum class WeatherType;

namespace WildWorldUtils
{
	// Generates the .INF name for the wilderness given a climate and current weather.
	std::string generateInfName(ClimateType climateType, WeatherType weatherType);
}

#endif
