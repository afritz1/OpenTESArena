#ifndef MUSIC_FILE_H
#define MUSIC_FILE_H

#include <string>

// Namespace for accessing Arena music filenames.

enum class ClimateType;
enum class LocationType;
enum class MusicName;
enum class WeatherType;

namespace MusicFile
{
	const std::string &fromName(MusicName musicName);
	MusicName fromWeather(WeatherType weatherType);
	MusicName jingleFromLocationAndClimate(LocationType locationType, ClimateType climateType);
}

#endif
