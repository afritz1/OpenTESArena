#ifndef CHASM_DEFINITION_H
#define CHASM_DEFINITION_H

#include <vector>

#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureAsset.h"

struct ChasmDefinition
{
	std::vector<TextureAsset> textureAssets; // Texture for each animation frame.
	bool allowsSwimming;
	bool isDamaging;

	ChasmDefinition();

	void initClassic(ArenaTypes::ChasmType chasmType);
};

#endif
