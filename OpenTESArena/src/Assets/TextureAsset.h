#pragma once

#include <optional>
#include <string>

// Points to a single texture on the file system. If the texture is part of a sequence of images,
// its position is stored in the index.
struct TextureAsset
{
	std::string filename;
	std::optional<int> index;

	constexpr TextureAsset(const std::string &filename, const std::optional<int> &index)
		: filename(filename), index(index) { }

	constexpr TextureAsset(const std::string &filename)
		: filename(filename) { }

	constexpr TextureAsset() { }

	bool operator==(const TextureAsset &other) const;
	bool operator!=(const TextureAsset &other) const;
};
