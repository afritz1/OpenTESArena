#ifndef WORLD_MAP_UI_VIEW_H
#define WORLD_MAP_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

class Game;

namespace WorldMapUiView
{
	constexpr double FastTravelAnimationSecondsPerFrame = 1.0 / 24.0;

	int getFastTravelAnimationTextureX(int textureWidth);
	int getFastTravelAnimationTextureY(int textureHeight);
	std::string getFastTravelAnimationFilename();
	TextureAssetReference getFastTravelPaletteTextureAssetRef();

	constexpr FontName CityArrivalFontName = FontName::Arena;
	const Color CityArrivalTextColor(251, 239, 77);
	constexpr TextAlignment CityArrivalTextAlignment = TextAlignment::Center;
	constexpr int CityArrivalLineSpacing = 1;
	constexpr TextureUtils::PatternType CityArrivalTexturePatternType = TextureUtils::PatternType::Dark;
	Int2 getCityArrivalPopUpTextCenterPoint(Game &game);
	Int2 getCityArrivalPopUpTextureCenterPoint(Game &game);
	int getCityArrivalPopUpTextureWidth(int textWidth);
	int getCityArrivalPopUpTextureHeight(int textHeight);
}

#endif
