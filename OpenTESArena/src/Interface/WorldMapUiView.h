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
	const Int2 BackToGameButtonCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH - 22,
		ArenaRenderUtils::SCREEN_HEIGHT - 7);
	constexpr int BackToGameButtonWidth = 36;
	constexpr int BackToGameButtonHeight = 9;

	Int2 getProvinceNameOffset(int provinceID, TextureManager &textureManager);

	TextureAssetReference getTextureAssetReference();
	TextureAssetReference getPaletteTextureAssetReference();
	std::string getProvinceNamesFilename();

	UiTextureID allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocHighlightedTextTexture(int provinceID, TextureManager &textureManager, Renderer &renderer);
}

namespace FastTravelUiView
{
	constexpr double AnimationSecondsPerFrame = 1.0 / 24.0;

	Int2 getAnimationTextureCenter();
	std::string getAnimationFilename();
	TextureAssetReference getPaletteTextureAssetRef();

	const std::string CityArrivalFontName = ArenaFontName::Arena;
	const Color CityArrivalTextColor(251, 239, 77);
	constexpr TextAlignment CityArrivalTextAlignment = TextAlignment::MiddleCenter;
	constexpr int CityArrivalLineSpacing = 1;
	constexpr TextureUtils::PatternType CityArrivalTexturePatternType = TextureUtils::PatternType::Dark;
	Int2 getCityArrivalPopUpTextCenterPoint(Game &game);
	Int2 getCityArrivalPopUpTextureCenterPoint(Game &game);
	int getCityArrivalPopUpTextureWidth(int textWidth);
	int getCityArrivalPopUpTextureHeight(int textHeight);

	UiTextureID allocCityArrivalPopUpTexture(int textWidth, int textHeight, TextureManager &textureManager, Renderer &renderer);
}

#endif
