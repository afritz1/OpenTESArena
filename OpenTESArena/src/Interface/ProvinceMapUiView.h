#ifndef PROVINCE_MAP_UI_VIEW_H
#define PROVINCE_MAP_UI_VIEW_H

#include <cstdint>

#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/ListBox.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

class Game;

struct TextureAssetReference;

namespace ProvinceMapUiView
{
	// -- Province panel --

	const Rect SearchButtonRect(34, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27);
	const Rect TravelButtonRect(53, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27);
	const Rect BackToWorldMapRect(72, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27);

	Int2 getLocationCenterPoint(Game &game, int provinceID, int locationID);
	const std::string LocationFontName = ArenaFontName::Arena;
	const Color LocationTextColor(158, 0, 0);
	constexpr TextAlignment LocationTextAlignment = TextAlignment::MiddleCenter;
	const Color LocationTextShadowColor(48, 48, 48);
	constexpr int LocationTextShadowOffsetX = 1;
	constexpr int LocationTextShadowOffsetY = 0;
	Int2 getLocationTextClampedPosition(int textX, int textY, int textWidth, int textHeight);

	const Int2 TextPopUpCenterPoint(ArenaRenderUtils::SCREEN_WIDTH / 2, 98);
	const std::string TextPopUpFontName = ArenaFontName::Arena;
	const Color TextPopUpTextColor(52, 24, 8);
	constexpr TextAlignment TextPopUpTextAlignment = TextAlignment::MiddleCenter;
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
	const std::string SearchSubPanelTitleFontName = ArenaFontName::Arena;
	const Color SearchSubPanelTitleColor(52, 24, 8);
	constexpr TextAlignment SearchSubPanelTitleTextAlignment = TextAlignment::TopLeft;

	TextBox::InitInfo getSearchSubPanelTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	const std::string SearchSubPanelTextEntryFontName = ArenaFontName::Arena;
	const Color SearchSubPanelTextEntryColor(52, 24, 8);
	constexpr TextAlignment SearchSubPanelTextEntryTextAlignment = TextAlignment::TopLeft;

	TextBox::InitInfo getSearchSubPanelTextEntryTextBoxInitInfo(const FontLibrary &fontLibrary);

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

	const Rect SearchSubPanelListBoxRect(85, 34, 147, 54);
	ListBox::Properties makeSearchSubPanelListBoxProperties(const FontLibrary &fontLibrary);

	TextureAssetReference getSearchSubPanelListTextureAssetRef();
	TextureAssetReference getSearchSubPanelListPaletteTextureAssetRef(Game &game, int provinceID);
}

#endif
