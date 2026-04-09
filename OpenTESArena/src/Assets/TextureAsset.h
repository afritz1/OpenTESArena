#pragma once

#include <string>

// Points to a single texture on the file system. If the texture is part of a sequence of images,
// its position is stored in the index.
struct TextureAsset
{
	std::string filename;
	int index;

	constexpr TextureAsset(const std::string &filename, int index)
		: filename(filename)
	{
		this->index = index;
	}

	constexpr TextureAsset(const std::string &filename)
		: filename(filename)
	{
		this->index = -1;
	}

	constexpr TextureAsset()
	{
		this->index = -1;
	}

	bool operator==(const TextureAsset &other) const;
	bool operator!=(const TextureAsset &other) const;
};
