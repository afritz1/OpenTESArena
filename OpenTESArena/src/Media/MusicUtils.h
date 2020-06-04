#ifndef MUSIC_UTILS_H
#define MUSIC_UTILS_H

#include <string_view>

#include "MusicDefinition.h"

// Various functions for working with the original game's music.

namespace MusicUtils
{
	// Gets the music type associated with a .MIF filename if it exists.
	bool tryGetInteriorMusicType(const std::string_view &mifName,
		MusicDefinition::InteriorMusicDefinition::Type *outType);
}

#endif
