#include "TextureMetadata.h"

void TextureMetadata::init(std::string &&filename, Buffer<Int2> &&dimensions)
{
	this->filename = std::move(filename);
	this->dimensions = std::move(dimensions);
}

void TextureMetadata::init(std::string &&filename, Buffer<Int2> &&dimensions, Buffer<Int2> &&offsets)
{
	this->init(std::move(filename), std::move(dimensions));
	this->offsets = std::move(offsets);
}

void TextureMetadata::init(std::string &&filename, Buffer<Int2> &&dimensions, double secondsPerFrame)
{
	this->init(std::move(filename), std::move(dimensions));
	this->secondsPerFrame = secondsPerFrame;
}

const std::string &TextureMetadata::getFilename() const
{
	return this->filename;
}

int TextureMetadata::getTextureCount() const
{
	return this->dimensions.getCount();
}

int TextureMetadata::getWidth(int index) const
{
	const Int2 &dims = this->dimensions.get(index);
	return dims.x;
}

int TextureMetadata::getHeight(int index) const
{
	const Int2 &dims = this->dimensions.get(index);
	return dims.y;
}

bool TextureMetadata::hasOffsets() const
{
	return this->offsets.getCount() > 0;
}

const Int2 &TextureMetadata::getOffset(int index) const
{
	DebugAssert(this->hasOffsets());
	return this->offsets.get(index);
}

bool TextureMetadata::isMovie() const
{
	return this->secondsPerFrame.has_value();
}

double TextureMetadata::getSecondsPerFrame() const
{
	DebugAssert(this->isMovie());
	return *this->secondsPerFrame;
}
