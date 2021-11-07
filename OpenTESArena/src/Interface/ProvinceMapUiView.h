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

class BinaryAssetLibrary;
class Game;

struct TextureAssetReference;

namespace ProvinceMapUiView
{
	enum class HighlightType
	{
		None,
		PlayerLocation,
		TravelDestination
	};

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
	Int2 getLocationTextClampedCenter(const Rect &unclampedRect);

	TextBox::InitInfo getHoveredLocationTextBoxInitInfo(const FontLibrary &fontLibrary);

	const Int2 TextPopUpCenterPoint(ArenaRenderUtils::SCREEN_WIDTH / 2, 98);
	const std::string TextPopUpFontName = ArenaFontName::Arena;
	const Color TextPopUpTextColor(52, 24, 8);
	constexpr TextAlignment TextPopUpTextAlignment = TextAlignment::MiddleCenter;
	constexpr int TextPopUpLineSpacing = 1;

	const Int2 TextPopUpTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	int getTextPopUpTextureWidth(int textWidth); // @todo: these should be merged into a plain old allocPopUpTexture()
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

	bool provinceHasStaffDungeonIcon(int provinceID);

	TextureAssetReference getBackgroundTextureAssetRef(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary);
	TextureAssetReference getBackgroundPaletteTextureAssetRef(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary);

	TextureAssetReference getCityStateIconTextureAssetRef(HighlightType highlightType);
	TextureAssetReference getTownIconTextureAssetRef(HighlightType highlightType);
	TextureAssetReference getVillageIconTextureAssetRef(HighlightType highlightType);
	TextureAssetReference getDungeonIconTextureAssetRef(HighlightType highlightType);
	TextureAssetReference getStaffDungeonIconTextureAssetRef(int provinceID);

	UiTextureID allocBackgroundTexture(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocCityStateIconTexture(HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef,
		TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocTownIconTexture(HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef, 
		TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocVillageIconTexture(HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef, 
		TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocDungeonIconTexture(HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef, 
		TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocStaffDungeonIconTexture(int provinceID, HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef, 
		TextureManager &textureManager, Renderer &renderer);
}

namespace ProvinceSearchUiView
{
	const Int2 DefaultTextCursorPosition(85, 100);

	const int TitleTextBoxX = 30;
	const int TitleTextBoxY = 89;
	const std::string TitleFontName = ArenaFontName::Arena;
	const Color TitleColor(52, 24, 8);
	constexpr TextAlignment TitleTextAlignment = TextAlignment::TopLeft;

	TextBox::InitInfo getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	const std::string TextEntryFontName = ArenaFontName::Arena;
	const Color TextEntryColor(52, 24, 8);
	constexpr TextAlignment TextEntryTextAlignment = TextAlignment::TopLeft;

	TextBox::InitInfo getTextEntryTextBoxInitInfo(const FontLibrary &fontLibrary);

	int getTextEntryTextureX(int textureWidth);
	int getTextEntryTextureY(int textureHeight);
	constexpr int TextureWidth = 280;
	constexpr int TextureHeight = 40;
	constexpr TextureUtils::PatternType TexturePattern = TextureUtils::PatternType::Parchment;

	const Int2 ListUpButtonCenterPoint(70, 24);
	constexpr int ListUpButtonWidth = 8;
	constexpr int ListUpButtonHeight = 8;

	const Int2 ListDownButtonCenterPoint(70, 97);
	constexpr int ListDownButtonWidth = 8;
	constexpr int ListDownButtonHeight = 8;

	constexpr int ListTextureX = 57;
	constexpr int ListTextureY = 11;

	const Rect ListBoxRect(85, 34, 147, 54);
	ListBox::Properties makeListBoxProperties(const FontLibrary &fontLibrary);

	TextureAssetReference getListTextureAssetRef();
	TextureAssetReference getListPaletteTextureAssetRef(Game &game, int provinceID);
}

#endif
