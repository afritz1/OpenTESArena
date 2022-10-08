#include <algorithm>

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

void ChasmDefinition::Animated::init(Buffer<TextureAsset> &&textureAssets)
{
	this->textureAssets = std::move(textureAssets);
}

ChasmDefinition::ChasmDefinition()
{
	this->allowsSwimming = false;
	this->isDamaging = false;
	this->animType = static_cast<AnimationType>(-1);
}

ChasmDefinition::ChasmDefinition(const ChasmDefinition &other)
{
	this->allowsSwimming = other.allowsSwimming;
	this->isDamaging = other.isDamaging;
	this->animType = other.animType;

	if (this->animType == AnimationType::SolidColor)
	{
		this->solidColor.init(other.solidColor.paletteIndex);
	}
	else if (this->animType == AnimationType::Animated)
	{
		const Buffer<TextureAsset> &otherTextureAssets = other.animated.textureAssets;
		Buffer<TextureAsset> textureAssets(otherTextureAssets.getCount());
		std::copy(otherTextureAssets.get(), otherTextureAssets.end(), textureAssets.get());
		this->animated.init(std::move(textureAssets));
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(this->animType)));
	}
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
		this->animated.init(ArenaChasmUtils::getTextureAssets(chasmType, textureManager));
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmType)));
	}
}