#include "TextureAsset.h"

TextureAsset::TextureAsset(std::string &&filename, const std::optional<int> &index)
	: filename(std::move(filename)), index(index) { }

TextureAsset::TextureAsset(std::string &&filename)
	: filename(std::move(filename)), index(std::nullopt) { }

TextureAsset::TextureAsset() { }

bool TextureAsset::operator==(const TextureAsset &other) const
{
	return (this->filename == other.filename) && (this->index == other.index);
}

bool TextureAsset::operator!=(const TextureAsset &other) const
{
	return (this->filename != other.filename) || (this->index != other.index);
}
