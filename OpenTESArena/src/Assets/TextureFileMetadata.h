#ifndef TEXTURE_FILE_METADATA_H
#define TEXTURE_FILE_METADATA_H

#include <optional>
#include <string>

#include "../Math/Vector2.h"

#include "components/utilities/Buffer.h"

// Various non-texel data about a texture file, useful when only worried about how many textures exist
// at that filename, as well as some header data (dimensions, screen offsets, etc.).

class TextureFileMetadata
{
private:
	std::string filename;
	Buffer<Int2> dimensions;

	// For .CFA + .CIF files.
	// @todo: to allow future texture formats like PNG to work with this, these values will need to come from a
	// helper file for the PNGs.
	Buffer<Int2> offsets;

	// For .FLC/.CEL animations.
	std::optional<double> secondsPerFrame;
public:
	void init(std::string &&filename, Buffer<Int2> &&dimensions);
	void init(std::string &&filename, Buffer<Int2> &&dimensions, Buffer<Int2> &&offsets);
	void init(std::string &&filename, Buffer<Int2> &&dimensions, double secondsPerFrame);

	const std::string &getFilename() const;
	int getTextureCount() const;
	int getWidth(int index) const;
	int getHeight(int index) const;

	bool hasOffsets() const;
	const Int2 &getOffset(int index) const;

	bool isMovie() const;
	double getSecondsPerFrame() const;
};

#endif
