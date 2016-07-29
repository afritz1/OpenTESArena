#ifndef TEXTURE_FILE_H
#define TEXTURE_FILE_H

#include <string>
#include <vector>

// This class takes care of mapping hardcoded texture names to filenames.
// Certain texture names are hardcoded because they should never change for the 
// lifetime of the application and are known beforehand.

// Once using the FLC and CEL movies, the texture sequences will represent the
// list of frames associated with each movie, instead of a list of PNG screenshots.
// So instead of getting an array of strings, a texture sequence name would get
// an array of SDL_Textures.

enum class TextureName;
enum class TextureSequenceName;

class TextureFile
{
private:
	TextureFile() = delete;
	TextureFile(const TextureFile&) = delete;
	~TextureFile() = delete;
public:
	// For texture manager sequence preloading.
	static std::vector<TextureSequenceName> getSequenceNames();

	static const std::string &fromName(TextureName textureName);
	static std::vector<std::string> fromName(TextureSequenceName sequenceName);
};

#endif
