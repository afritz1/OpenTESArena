#include "TextureFileMetadata.h"

TextureFileMetadata::TextureFileMetadata(std::string &&filename, Buffer<Int2> &&dimensions, Buffer<Int2> &&offsets)
	: filename(std::move(filename)), dimensions(std::move(dimensions)), offsets(std::move(offsets)) { }

TextureFileMetadata::TextureFileMetadata(std::string &&filename, Buffer<Int2> &&dimensions)
	: filename(std::move(filename)), dimensions(std::move(dimensions)), offsets(Buffer<Int2>()) { }

const std::string &TextureFileMetadata::getFilename() const
{
	return this->filename;
}

int TextureFileMetadata::getTextureCount() const
{
	return this->dimensions.getCount();
}

int TextureFileMetadata::getWidth(int index) const
{
	const Int2 &dims = this->dimensions.get(index);
	return dims.x;
}

int TextureFileMetadata::getHeight(int index) const
{
	const Int2 &dims = this->dimensions.get(index);
	return dims.y;
}

bool TextureFileMetadata::hasOffsets() const
{
	return this->offsets.getCount() > 0;
}

const Int2 &TextureFileMetadata::getOffset(int index)
{
	DebugAssert(this->hasOffsets());
	return this->offsets.get(index);
}
