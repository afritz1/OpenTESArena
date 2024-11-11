#ifndef TEXTURE_ASSET_H
#define TEXTURE_ASSET_H

#include <optional>
#include <string>

// General-purpose reference for a single texture. The index is set if the filename points to a
// sequence of individual textures.
struct TextureAsset
{
	std::string filename;
	std::optional<int> index; // Points into sequential texture file.

	TextureAsset(std::string &&filename, const std::optional<int> &index);
	TextureAsset(std::string &&filename);
	TextureAsset();

	bool operator==(const TextureAsset &other) const;
	bool operator!=(const TextureAsset &other) const;
};

#endif
