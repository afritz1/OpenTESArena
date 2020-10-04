#ifndef CITY_WORLD_UTILS_H
#define CITY_WORLD_UTILS_H

#include "components/dos/DOSUtils.h"

enum class ClimateType;
enum class WeatherType;

namespace CityWorldUtils
{
	// Generates the .INF name for a city given a climate and current weather.
	DOSUtils::FilenameBuffer generateInfName(ClimateType climateType, WeatherType weatherType);
}

#endif
