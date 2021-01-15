#ifndef TEXTURE_ASSET_REFERENCE_H
#define TEXTURE_ASSET_REFERENCE_H

#include <optional>
#include <string>

// General-purpose reference for a single texture. The index is set if the filename points to a
// sequence of individual textures.

struct TextureAssetReference
{
	std::string filename;
	std::optional<int> index; // Points into sequential texture file.

	TextureAssetReference(std::string &&filename, const std::optional<int> &index);
	TextureAssetReference(std::string &&filename);
	TextureAssetReference();

	bool operator==(const TextureAssetReference &other) const;
};

#endif
