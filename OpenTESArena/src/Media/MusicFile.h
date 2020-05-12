#ifndef MUSIC_FILE_H
#define MUSIC_FILE_H

#include <string>

#include "../World/LocationDefinition.h"

// Namespace for accessing Arena music filenames.

enum class ClimateType;
enum class MusicName;
enum class WeatherType;

namespace MusicFile
{
	const std::string &fromName(MusicName musicName);
	MusicName fromWeather(WeatherType weatherType);
	MusicName jingleFromCityTypeAndClimate(LocationDefinition::CityDefinition::Type locationType,
		ClimateType climateType);
}

#endif
