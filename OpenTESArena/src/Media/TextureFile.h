#ifndef TEXTURE_FILE_H
#define TEXTURE_FILE_H

#include <string>

// This class takes care of mapping hardcoded texture names to filenames.
// Certain texture names are hardcoded because they should never change for the 
// lifetime of the application and are known beforehand.

// The texture sequences represent the list of frames associated with each movie.

enum class TextureName;
enum class TextureSequenceName;

class TextureFile
{
private:
	TextureFile() = delete;
	TextureFile(const TextureFile&) = delete;
	~TextureFile() = delete;
public:
	static const std::string &fromName(TextureName textureName);
	static const std::string &fromName(TextureSequenceName sequenceName);
};

#endif
