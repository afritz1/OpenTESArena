#include "TextureFileMetadata.h"

TextureFileMetadata::TextureFileMetadata(std::string &&filename, Buffer<std::pair<int, int>> &&dimensions)
	: filename(std::move(filename)), dimensions(std::move(dimensions)) { }

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
	const auto &pair = this->dimensions.get(index);
	return pair.first;
}

int TextureFileMetadata::getHeight(int index) const
{
	const auto &pair = this->dimensions.get(index);
	return pair.second;
}
