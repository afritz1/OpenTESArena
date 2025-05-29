#ifndef ARENA_CHASM_UTILS_H
#define ARENA_CHASM_UTILS_H

#include <string>

#include "../Assets/ArenaTypes.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

class TextureManager;

namespace ArenaChasmUtils
{
	// Water and lava chasms use this instead of ceiling scale.
	constexpr double DEFAULT_HEIGHT = static_cast<double>(INFCeiling::DEFAULT_HEIGHT) / MIFUtils::ARENA_UNITS;

	bool isTextured(ArenaChasmType chasmType);
	bool allowsSwimming(ArenaChasmType chasmType);
	bool isDamaging(ArenaChasmType chasmType);
	Buffer<TextureAsset> getTextureAssets(ArenaChasmType chasmType, TextureManager &textureManager);
}

#endif
