#ifndef MUSIC_UTILS_H
#define MUSIC_UTILS_H

#include "MusicDefinition.h"
#include "../Assets/ArenaTypes.h"

// Various functions for working with the original game's music.

namespace MusicUtils
{
	// Gets the music type associated with an interior.
	MusicDefinition::InteriorMusicDefinition::Type getInteriorMusicType(ArenaTypes::InteriorType interiorType);
}

#endif
