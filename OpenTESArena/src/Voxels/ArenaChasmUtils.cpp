#include "ArenaChasmUtils.h"
#include "../Assets/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

bool ArenaChasmUtils::isTextured(ArenaChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaChasmType::Dry:
		return false;
	case ArenaChasmType::Wet:
	case ArenaChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

bool ArenaChasmUtils::allowsSwimming(ArenaChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaChasmType::Dry:
		return false;
	case ArenaChasmType::Wet:
	case ArenaChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

bool ArenaChasmUtils::isDamaging(ArenaChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaChasmType::Dry:
	case ArenaChasmType::Wet:
		return false;
	case ArenaChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

Buffer<TextureAsset> ArenaChasmUtils::getTextureAssets(ArenaChasmType chasmType, TextureManager &textureManager)
{
	if (!ArenaChasmUtils::isTextured(chasmType))
	{
		return Buffer<TextureAsset>();
	}

	const std::string &filename = [chasmType]()
	{
		switch (chasmType)
		{
		case ArenaChasmType::Wet:
			return ArenaRenderUtils::CHASM_WATER_FILENAME;
		case ArenaChasmType::Lava:
			return ArenaRenderUtils::CHASM_LAVA_FILENAME;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmType)));
		}
	}();

	return TextureUtils::makeTextureAssets(filename, textureManager);
}
