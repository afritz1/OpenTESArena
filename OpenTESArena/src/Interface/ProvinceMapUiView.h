#ifndef PROVINCE_MAP_UI_VIEW_H
#define PROVINCE_MAP_UI_VIEW_H

#include <cstdint>

#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

class Game;

struct TextureAssetReference;

namespace ProvinceMapUiView
{
	// -- Province panel --

	const Rect SearchButtonRect(34, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27);
	const Rect TravelButtonRect(53, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27);
	const Rect BackToWorldMapRect(72, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27);

	Int2 getLocationCenterPoint(Game &game, int provinceID, int locationID);
	constexpr FontName LocationFontName = FontName::Arena;
	const Color LocationTextColor(158, 0, 0);
	constexpr TextAlignment LocationTextAlignment = TextAlignment::Center;
	const Color LocationTextShadowColor(48, 48, 48);
	const Int2 LocationTextShadowOffset(1, 0);
	Int2 getLocationTextClampedPosition(int textX, int textY, int textWidth, int textHeight);

	const Int2 TextPopUpCenterPoint(ArenaRenderUtils::SCREEN_WIDTH / 2, 98);
	constexpr FontName TextPopUpFontName = FontName::Arena;
	const Color TextPopUpTextColor(52, 24, 8);
	constexpr TextAlignment TextPopUpTextAlignment = TextAlignment::Center;
	constexpr int TextPopUpLineSpacing = 1;

	const Int2 TextPopUpTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	int getTextPopUpTextureWidth(int textWidth);
	int getTextPopUpTextureHeight(int textHeight);

	constexpr TextureUtils::PatternType TextPopUpTexturePatternType = TextureUtils::PatternType::Parchment;

	constexpr double BlinkPeriodSeconds = 1.0 / 5.0; // Duration of entire blink period in seconds.
	constexpr double BlinkPeriodPercentOn = 0.75; // Percentage of each period spent "on".

	// .CIF palette indices for staff dungeon outlines.
	constexpr uint8_t BackgroundPaletteIndex = 220;
	constexpr uint8_t YellowPaletteIndex = 194;
	constexpr uint8_t RedPaletteIndex = 223;

	constexpr int CityStateIconHighlightIndex = 0;
	constexpr int TownIconHighlightIndex = 1;
	constexpr int VillageIconHighlightIndex = 2;
	constexpr int DungeonIconHighlightIndex = 3;

	TextureAssetReference getProvinceBackgroundTextureAssetRef(Game &game, int provinceID);
	TextureAssetReference getProvinceBackgroundPaletteTextureAssetRef(Game &game, int provinceID);

	TextureAssetReference getCityStateIconTextureAssetRef();
	TextureAssetReference getTownIconTextureAssetRef();
	TextureAssetReference getVillageIconTextureAssetRef();
	TextureAssetReference getDungeonIconTextureAssetRef();
	std::string getStaffDungeonIconsFilename();
	TextureAssetReference getStaffDungeonIconTextureAssetRef(int provinceID);

	std::string getMapIconOutlinesFilename();
	std::string getMapIconBlinkingOutlinesFilename();

	// -- Search sub-panel --

	const Int2 SearchSubPanelDefaultTextCursorPosition(85, 100);

	const int SearchSubPanelTitleTextBoxX = 30;
	const int SearchSubPanelTitleTextBoxY = 89;
	constexpr FontName SearchSubPanelTitleFontName = FontName::Arena;
	const Color SearchSubPanelTitleColor(52, 24, 8);
	constexpr TextAlignment SearchSubPanelTitleTextAlignment = TextAlignment::Left;

	constexpr FontName SearchSubPanelTextEntryFontName = FontName::Arena;
	const Color SearchSubPanelTextEntryColor(52, 24, 8);
	constexpr TextAlignment SearchSubPanelTextEntryTextAlignment = TextAlignment::Left;

	int getSearchSubPanelTextEntryTextureX(int textureWidth);
	int getSearchSubPanelTextEntryTextureY(int textureHeight);
	constexpr int SearchSubPanelTextureWidth = 280;
	constexpr int SearchSubPanelTextureHeight = 40;
	constexpr TextureUtils::PatternType SearchSubPanelTexturePattern = TextureUtils::PatternType::Parchment;

	const Int2 SearchSubPanelListUpButtonCenterPoint(70, 24);
	constexpr int SearchSubPanelListUpButtonWidth = 8;
	constexpr int SearchSubPanelListUpButtonHeight = 8;

	const Int2 SearchSubPanelListDownButtonCenterPoint(70, 97);
	constexpr int SearchSubPanelListDownButtonWidth = 8;
	constexpr int SearchSubPanelListDownButtonHeight = 8;

	constexpr int SearchSubPanelListTextureX = 57;
	constexpr int SearchSubPanelListTextureY = 11;

	constexpr int SearchSubPanelListBoxX = 85;
	constexpr int SearchSubPanelListBoxY = 34;
	constexpr int SearchSubPanelListBoxWidth = 147; // Only width here since it is a custom fit to occupy more space.
	constexpr FontName SearchSubPanelListBoxFontName = FontName::Arena;
	const Color SearchSubPanelListBoxTextColor(52, 24, 8);
	constexpr int SearchSubPanelListBoxMaxDisplayed = 6;

	TextureAssetReference getSearchSubPanelListTextureAssetRef();
	TextureAssetReference getSearchSubPanelListPaletteTextureAssetRef(Game &game, int provinceID);
}

#endif
