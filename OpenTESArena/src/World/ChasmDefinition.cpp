#include "ArenaChasmUtils.h"
#include "ChasmDefinition.h"
#include "../Rendering/ArenaRenderUtils.h"

ChasmDefinition::SolidColor::SolidColor()
{
	this->paletteIndex = 0;
}

void ChasmDefinition::SolidColor::init(uint8_t paletteIndex)
{
	this->paletteIndex = paletteIndex;
}

ChasmDefinition::ChasmDefinition()
{
	this->allowsSwimming = false;
	this->isDamaging = false;
	this->animType = static_cast<AnimationType>(-1);
}

void ChasmDefinition::initClassic(ArenaTypes::ChasmType chasmType, TextureManager &textureManager)
{
	this->allowsSwimming = ArenaChasmUtils::allowsSwimming(chasmType);
	this->isDamaging = ArenaChasmUtils::isDamaging(chasmType);

	if (chasmType == ArenaTypes::ChasmType::Dry)
	{
		this->animType = AnimationType::SolidColor;
		this->solidColor.init(ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR);
	}
	else if ((chasmType == ArenaTypes::ChasmType::Wet) || (chasmType == ArenaTypes::ChasmType::Lava))
	{
		this->animType = AnimationType::Animated;
		this->animated.textureAssets = ArenaChasmUtils::getTextureAssets(chasmType, textureManager);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmType)));
	}
}
