#ifndef MUSIC_FILE_H
#define MUSIC_FILE_H

#include <string>

// Static class for accessing Arena music filenames.

enum class MusicName;
enum class WeatherType;

class MusicFile
{
private:
	MusicFile() = delete;
	~MusicFile() = delete;
public:
	static const std::string &fromName(MusicName musicName);
	static MusicName fromWeather(WeatherType weatherType);
};

#endif
