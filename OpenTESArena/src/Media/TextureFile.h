#ifndef TEXTURE_FILE_H
#define TEXTURE_FILE_H

#include <string>

// This namespace takes care of mapping hardcoded texture names to filenames.
// Certain texture names are hardcoded because they should never change for the 
// lifetime of the application and are known beforehand.

// The texture sequences represent the list of frames associated with each movie.

enum class TextureName;
enum class TextureSequenceName;

namespace TextureFile
{
	const std::string &fromName(TextureName textureName);
	const std::string &fromName(TextureSequenceName sequenceName);
}

#endif
