#ifndef CHASM_DEFINITION_H
#define CHASM_DEFINITION_H

#include <cstdint>

#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

class TextureManager;

enum class VoxelChasmAnimationType
{
	SolidColor,
	Animated // Screen-space texture.
};

struct VoxelChasmSolidColor
{
	uint8_t paletteIndex;

	VoxelChasmSolidColor();

	void init(uint8_t paletteIndex);
};

struct VoxelChasmAnimated
{
	Buffer<TextureAsset> textureAssets; // Texture for each animation frame.

	void init(Buffer<TextureAsset> &&textureAssets);
};

struct VoxelChasmDefinition
{
	bool allowsSwimming;
	bool isDamaging;
	bool isEmissive;
	TextureAsset wallTextureAsset;

	VoxelChasmAnimationType animType; // Determines solid color/animated access.
	VoxelChasmSolidColor solidColor;
	VoxelChasmAnimated animated;

	VoxelChasmDefinition();
	VoxelChasmDefinition(const VoxelChasmDefinition &other);

	void initClassic(ArenaTypes::ChasmType chasmType, const TextureAsset &wallTextureAsset, TextureManager &textureManager);
};

#endif
