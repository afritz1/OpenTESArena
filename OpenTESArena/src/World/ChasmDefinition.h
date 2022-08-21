#ifndef CHASM_DEFINITION_H
#define CHASM_DEFINITION_H

#include <cstdint>

#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

class TextureManager;

struct ChasmDefinition
{
	enum class AnimationType
	{
		SolidColor,
		Animated // Screen-space texture.
	};

	struct SolidColor
	{
		uint8_t paletteIndex;

		SolidColor();

		void init(uint8_t paletteIndex);
	};

	struct Animated
	{
		Buffer<TextureAsset> textureAssets; // Texture for each animation frame.

		void init(Buffer<TextureAsset> &&textureAssets);
	};
	
	bool allowsSwimming;
	bool isDamaging;

	AnimationType animType; // Determines solid color/animated access.
	SolidColor solidColor;
	Animated animated;

	ChasmDefinition();
	ChasmDefinition(const ChasmDefinition &other);

	void initClassic(ArenaTypes::ChasmType chasmType, TextureManager &textureManager);
};

#endif
