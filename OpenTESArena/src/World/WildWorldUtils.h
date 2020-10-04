#ifndef WILD_WORLD_UTILS_H
#define WILD_WORLD_UTILS_H

#include "components/dos/DOSUtils.h"

enum class ClimateType;
enum class WeatherType;

namespace WildWorldUtils
{
	// Generates the .INF name for the wilderness given a climate and current weather.
	DOSUtils::FilenameBuffer generateInfName(ClimateType climateType, WeatherType weatherType);
}

#endif
