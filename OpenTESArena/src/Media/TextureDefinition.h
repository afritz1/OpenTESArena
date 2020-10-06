#ifndef TEXTURE_DEFINITION_H
#define TEXTURE_DEFINITION_H

#include <optional>
#include <string>

// General-purpose texture reference to the file system. Use this if trying to keep runtime texture
// handles out of implementation details.

class TextureDefinition
{
private:
	std::string filename;
	std::optional<int> index; // Optional index into multi-texture file.
public:
	TextureDefinition(std::string &&filename, int index);
	TextureDefinition(std::string &&filename);
	TextureDefinition();

	const std::string &getFilename() const;
	const std::optional<int> &getIndex() const;

	void setFilename(std::string &&filename);
	void setIndex(int index);
	void resetIndex();
};

#endif
