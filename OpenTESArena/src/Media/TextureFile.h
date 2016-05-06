#ifndef TEXTURE_FILE_H
#define TEXTURE_FILE_H

#include <string>
#include <vector>

// This class takes care of mapping hardcoded texture names to filenames.
// Certain texture names are hardcoded because they should never change for the 
// lifetime of the application and are known beforehand.

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

	static std::string fromName(TextureName textureName);
	static std::vector<std::string> fromName(TextureSequenceName sequenceName);
};

#endif
