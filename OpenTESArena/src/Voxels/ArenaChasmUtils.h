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
	constexpr double DEFAULT_HEIGHT = static_cast<double>(INFFile::CeilingData::DEFAULT_HEIGHT) / MIFUtils::ARENA_UNITS;

	bool isTextured(ArenaTypes::ChasmType chasmType);
	bool allowsSwimming(ArenaTypes::ChasmType chasmType);
	bool isDamaging(ArenaTypes::ChasmType chasmType);
	Buffer<TextureAsset> getTextureAssets(ArenaTypes::ChasmType chasmType, TextureManager &textureManager);
}

#endif
