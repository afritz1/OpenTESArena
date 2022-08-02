#ifndef ARENA_CHASM_UTILS_H
#define ARENA_CHASM_UTILS_H

#include <string>

#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

class TextureManager;

namespace ArenaChasmUtils
{
	bool isTextured(ArenaTypes::ChasmType chasmType);
	bool allowsSwimming(ArenaTypes::ChasmType chasmType);
	bool isDamaging(ArenaTypes::ChasmType chasmType);
	Buffer<TextureAsset> getTextureAssets(ArenaTypes::ChasmType chasmType, TextureManager &textureManager);
}

#endif
