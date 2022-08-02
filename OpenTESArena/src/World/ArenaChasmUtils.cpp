#include "ArenaChasmUtils.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

bool ArenaChasmUtils::isTextured(ArenaTypes::ChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaTypes::ChasmType::Dry:
		return false;
	case ArenaTypes::ChasmType::Wet:
	case ArenaTypes::ChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

bool ArenaChasmUtils::allowsSwimming(ArenaTypes::ChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaTypes::ChasmType::Dry:
		return false;
	case ArenaTypes::ChasmType::Wet:
	case ArenaTypes::ChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

bool ArenaChasmUtils::isDamaging(ArenaTypes::ChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaTypes::ChasmType::Dry:
	case ArenaTypes::ChasmType::Wet:
		return false;
	case ArenaTypes::ChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

Buffer<TextureAsset> ArenaChasmUtils::getTextureAssets(ArenaTypes::ChasmType chasmType, TextureManager &textureManager)
{
	if (!ArenaChasmUtils::isTextured(chasmType))
	{
		return Buffer<TextureAsset>();
	}

	const std::string &filename = [chasmType]()
	{
		switch (chasmType)
		{
		case ArenaTypes::ChasmType::Wet:
			return ArenaRenderUtils::CHASM_WATER_FILENAME;
		case ArenaTypes::ChasmType::Lava:
			return ArenaRenderUtils::CHASM_LAVA_FILENAME;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmType)));
		}
	}();

	return TextureUtils::makeTextureAssets(filename, textureManager);
}
