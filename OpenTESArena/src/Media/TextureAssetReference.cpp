#include "TextureAssetReference.h"

TextureAssetReference::TextureAssetReference(std::string &&filename, int index)
	: filename(std::move(filename)), index(index) { }

TextureAssetReference::TextureAssetReference(std::string &&filename)
	: filename(std::move(filename)), index(std::nullopt) { }

TextureAssetReference::TextureAssetReference() { }
