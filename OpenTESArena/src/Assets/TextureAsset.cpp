#include "TextureAsset.h"

bool TextureAsset::operator==(const TextureAsset &other) const
{
	return (this->filename == other.filename) && (this->index == other.index);
}

bool TextureAsset::operator!=(const TextureAsset &other) const
{
	return (this->filename != other.filename) || (this->index != other.index);
}
