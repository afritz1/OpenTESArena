#ifndef WORLD_MAP_UI_VIEW_H
#define WORLD_MAP_UI_VIEW_H

#include <string>

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"

class Game;

namespace WorldMapUiView
{
	// -- World map --

	const Int2 BackToGameButtonCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH - 22,
		ArenaRenderUtils::SCREEN_HEIGHT - 7);
	constexpr int BackToGameButtonWidth = 36;
	constexpr int BackToGameButtonHeight = 9;

	TextureAssetReference getWorldMapTextureAssetReference();
	TextureAssetReference getWorldMapPaletteTextureAssetReference();
	std::string getProvinceNamesFilename();

	// -- Fast travel --

	constexpr double FastTravelAnimationSecondsPerFrame = 1.0 / 24.0;

	int getFastTravelAnimationTextureX(int textureWidth);
	int getFastTravelAnimationTextureY(int textureHeight);
	std::string getFastTravelAnimationFilename();
	TextureAssetReference getFastTravelPaletteTextureAssetRef();

	const std::string CityArrivalFontName = ArenaFontName::Arena;
	const Color CityArrivalTextColor(251, 239, 77);
	constexpr TextAlignment CityArrivalTextAlignment = TextAlignment::MiddleCenter;
	constexpr int CityArrivalLineSpacing = 1;
	constexpr TextureUtils::PatternType CityArrivalTexturePatternType = TextureUtils::PatternType::Dark;
	Int2 getCityArrivalPopUpTextCenterPoint(Game &game);
	Int2 getCityArrivalPopUpTextureCenterPoint(Game &game);
	int getCityArrivalPopUpTextureWidth(int textWidth);
	int getCityArrivalPopUpTextureHeight(int textHeight);
}

#endif
