#ifndef VOXEL_TEXTURE_DEFINITION_H
#define VOXEL_TEXTURE_DEFINITION_H

#include "../Assets/TextureAsset.h"

struct VoxelTextureDefinition
{
	static constexpr int MAX_TEXTURES = 3; // To support worst-case voxels like walls with different side/top/bottom.

	TextureAsset textureAssets[MAX_TEXTURES];
	int textureCount;

	VoxelTextureDefinition();

	const TextureAsset &getTextureAsset(int index) const;
	void addTextureAsset(TextureAsset &&textureAsset);
};

#endif
