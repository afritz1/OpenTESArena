#ifndef MUSIC_UTILS_H
#define MUSIC_UTILS_H

#include "MusicDefinition.h"
#include "../Assets/ArenaTypes.h"

class Random;

struct Clock;
struct WeatherDefinition;

// Various functions for working with the original game's music.
namespace MusicUtils
{
	// Gets the music type associated with an interior.
	InteriorMusicType getInteriorMusicType(ArenaInteriorType interiorType);

	const MusicDefinition *getExteriorMusicDefinition(const WeatherDefinition &weatherDef, const Clock &clock, Random &random);
	const MusicDefinition *getRandomDungeonMusicDefinition(Random &random);
	const MusicDefinition *getMainQuestCinematicGoodMusicDefinition(Random &random);
}

#endif
