#include "TextureAssetReference.h"

TextureAssetReference::TextureAssetReference(std::string &&filename, const std::optional<int> &index)
	: filename(std::move(filename)), index(index) { }

TextureAssetReference::TextureAssetReference(std::string &&filename)
	: filename(std::move(filename)), index(std::nullopt) { }

TextureAssetReference::TextureAssetReference() { }

bool TextureAssetReference::operator==(const TextureAssetReference &other) const
{
	return (this->filename == other.filename) && (this->index == other.index);
}
