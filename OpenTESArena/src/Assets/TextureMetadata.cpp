#include <algorithm>

#include "TextureMetadata.h"

void TextureMetadata::init(const std::string &filename, Span<const Int2> dimensions)
{
	this->filename = filename;
	this->dimensions.init(dimensions.getCount());
	std::copy(dimensions.begin(), dimensions.end(), this->dimensions.begin());
}

void TextureMetadata::init(const std::string &filename, Span<const Int2> dimensions, Span<const Int2> offsets)
{
	this->init(filename, dimensions);
	this->offsets.init(offsets.getCount());
	std::copy(offsets.begin(), offsets.end(), this->offsets.begin());
}

void TextureMetadata::init(const std::string &filename, Span<const Int2> dimensions, double secondsPerFrame)
{
	this->init(filename, dimensions);
	this->secondsPerFrame = secondsPerFrame;
}

int TextureMetadata::getTextureCount() const
{
	return this->dimensions.getCount();
}

int TextureMetadata::getWidth(int index) const
{
	const Int2 dims = this->dimensions.get(index);
	return dims.x;
}

int TextureMetadata::getHeight(int index) const
{
	const Int2 dims = this->dimensions.get(index);
	return dims.y;
}

bool TextureMetadata::hasOffsets() const
{
	return this->offsets.getCount() > 0;
}

bool TextureMetadata::isMovie() const
{
	return this->secondsPerFrame > 0.0;
}
