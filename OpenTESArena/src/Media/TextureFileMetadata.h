#ifndef TEXTURE_FILE_METADATA_H
#define TEXTURE_FILE_METADATA_H

#include <string>

#include "components/utilities/Buffer.h"

// Various non-texel data about a texture file, useful when only worried about how many textures exist
// at that filename and what their dimensions are.

class TextureFileMetadata
{
private:
	std::string filename;
	Buffer<std::pair<int, int>> dimensions;
public:
	TextureFileMetadata(std::string &&filename, Buffer<std::pair<int, int>> &&dimensions);

	const std::string &getFilename() const;
	int getTextureCount() const;
	int getWidth(int index) const;
	int getHeight(int index) const;
};

#endif
