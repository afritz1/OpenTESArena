#ifndef MUSIC_UTILS_H
#define MUSIC_UTILS_H

#include <string>

class Random;

enum class MusicName;
enum class WeatherType;

// Various functions for working with the original game's music.

namespace MusicUtils
{
	// Gets the music name associated with the given weather. The caller may need to check
	// the current time to see if they should use night music instead.
	MusicName getExteriorMusicName(WeatherType weatherType);

	// Gets a random dungeon music name.
	MusicName getDungeonMusicName(Random &random);

	// Gets the music name associated with a .MIF filename. If the selection involves
	// choosing from a list, the RNG will be used.
	MusicName getInteriorMusicName(const std::string &mifName, Random &random);
}

#endif
