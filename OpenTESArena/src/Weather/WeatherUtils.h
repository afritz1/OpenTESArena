#ifndef WEATHER_UTILS_H
#define WEATHER_UTILS_H

#include "../Assets/ArenaTypes.h"

#include "components/utilities/Buffer.h"

class Random;
class WeatherDefinition;

enum class MapType;

namespace WeatherUtils
{
	Buffer<WeatherDefinition> makeInteriorDefs();

	// Generates a set of allowed weathers. The given RNG is cosmetic for determining if e.g. there should be a thunderstorm.
	Buffer<WeatherDefinition> makeExteriorDefs(ArenaTypes::ClimateType climateType, int currentDay, Random &random);
}

#endif
