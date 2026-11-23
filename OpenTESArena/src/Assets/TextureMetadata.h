#pragma once

#include <string>

#include "../Math/Vector2.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Span.h"

// Various non-texel data about a texture file, useful when only worried about how many textures exist
// at that filename, as well as some header data (dimensions, screen offsets, etc.).
struct TextureMetadata
{
	std::string filename;
	Buffer<Int2> dimensions;

	// For .CFA + .CIF files.
	// @todo: to allow future texture formats like PNG to work with this, these values will need to come from a
	// helper file for the PNGs.
	Buffer<Int2> offsets;

	// For .FLC/.CEL animations.
	double secondsPerFrame;

	void init(const std::string &filename, Span<const Int2> dimensions);
	void init(const std::string &filename, Span<const Int2> dimensions, Span<const Int2> offsets);
	void init(const std::string &filename, Span<const Int2> dimensions, double secondsPerFrame);

	int getTextureCount() const;
	int getWidth(int index) const;
	int getHeight(int index) const;

	bool hasOffsets() const;

	bool isMovie() const;
};
