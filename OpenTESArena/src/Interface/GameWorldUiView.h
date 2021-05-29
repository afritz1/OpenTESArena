#ifndef GAME_WORLD_UI_VIEW_H
#define GAME_WORLD_UI_VIEW_H

#include <array>

#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/CursorAlignment.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

class Game;

namespace GameWorldUiView
{
	// Original arrow cursor rectangles for each part of the letterbox. Their components can be multiplied by
	// the ratio of the native and the original resolution so they're flexible with most resolutions.
	const Rect TopLeftRegion(0, 0, 141, 49);
	const Rect TopMiddleRegion(141, 0, 38, 49);
	const Rect TopRightRegion(179, 0, 141, 49);
	const Rect MiddleLeftRegion(0, 49, 90, 70);
	const Rect MiddleRegion(90, 49, 140, 70);
	const Rect MiddleRightRegion(230, 49, 90, 70);
	const Rect BottomLeftRegion(0, 119, 141, 28);
	const Rect BottomMiddleRegion(141, 119, 38, 28);
	const Rect BottomRightRegion(179, 119, 141, 28);
	const Rect UiBottomRegion(0, 147, 320, 53);

	// Arrow cursor alignments. These offset the drawn cursor relative to the mouse position so the cursor's
	// click area is closer to the tip of each arrow, as is done in the original game (slightly differently,
	// though. I think the middle cursor was originally top-aligned, not middle-aligned, which is strange).
	constexpr std::array<CursorAlignment, 9> ArrowCursorAlignments =
	{
		CursorAlignment::TopLeft,
		CursorAlignment::Top,
		CursorAlignment::TopRight,
		CursorAlignment::TopLeft,
		CursorAlignment::Middle,
		CursorAlignment::TopRight,
		CursorAlignment::Left,
		CursorAlignment::Bottom,
		CursorAlignment::Right
	};

	constexpr int PlayerNameTextBoxX = 17;
	constexpr int PlayerNameTextBoxY = 154;
	constexpr FontName PlayerNameFontName = FontName::Char;
	const Color PlayerNameTextColor(215, 121, 8);
	constexpr TextAlignment PlayerNameTextAlignment = TextAlignment::Left;

	constexpr int CharacterSheetButtonX = 14;
	constexpr int CharacterSheetButtonY = 166;
	constexpr int CharacterSheetButtonWidth = 40;
	constexpr int CharacterSheetButtonHeight = 29;

	constexpr int WeaponSheathButtonX = 88;
	constexpr int WeaponSheathButtonY = 151;
	constexpr int WeaponSheathButtonWidth = 29;
	constexpr int WeaponSheathButtonHeight = 22;

	constexpr int StealButtonX = 147;
	constexpr int StealButtonY = 151;
	constexpr int StealButtonWidth = 29;
	constexpr int StealButtonHeight = 22;

	constexpr int StatusButtonX = 177;
	constexpr int StatusButtonY = 151;
	constexpr int StatusButtonWidth = 29;
	constexpr int StatusButtonHeight = 22;
	
	constexpr FontName StatusPopUpFontName = FontName::Arena;
	const Color StatusPopUpTextColor(251, 239, 77);
	constexpr TextAlignment StatusPopUpTextAlignment = TextAlignment::Center;
	constexpr int StatusPopUpTextLineSpacing = 1;

	constexpr TextureUtils::PatternType StatusPopUpTexturePatternType = TextureUtils::PatternType::Dark;

	Int2 getStatusPopUpTextCenterPoint(Game &game);
	int getStatusPopUpTextureWidth(int textWidth);
	int getStatusPopUpTextureHeight(int textHeight);

	constexpr int MagicButtonX = 88;
	constexpr int MagicButtonY = 175;
	constexpr int MagicButtonWidth = 29;
	constexpr int MagicButtonHeight = 22;

	constexpr int LogbookButtonX = 118;
	constexpr int LogbookButtonY = 175;
	constexpr int LogbookButtonWidth = 29;
	constexpr int LogbookButtonHeight = 22;

	constexpr int UseItemButtonX = 147;
	constexpr int UseItemButtonY = 175;
	constexpr int UseItemButtonWidth = 29;
	constexpr int UseItemButtonHeight = 22;

	constexpr int CampButtonX = 177;
	constexpr int CampButtonY = 175;
	constexpr int CampButtonWidth = 29;
	constexpr int CampButtonHeight = 22;
	
	constexpr int ScrollUpButtonX = 208;
	constexpr int ScrollUpButtonY = (ArenaRenderUtils::SCREEN_HEIGHT - 53) + 3;
	constexpr int ScrollUpButtonWidth = 9;
	constexpr int ScrollUpButtonHeight = ScrollUpButtonWidth;

	constexpr int ScrollDownButtonX = 208;
	constexpr int ScrollDownButtonY = (ArenaRenderUtils::SCREEN_HEIGHT - 53) + 44;
	constexpr int ScrollDownButtonWidth = 9;
	constexpr int ScrollDownButtonHeight = ScrollDownButtonWidth;

	constexpr int MapButtonX = 118;
	constexpr int MapButtonY = 151;
	constexpr int MapButtonWidth = 29;
	constexpr int MapButtonHeight = 22;

	Int2 getCurrentWeaponAnimationOffset(Game &game);

	// Gets the center of the screen for pop-up related functions. The position depends on
	// whether modern interface mode is set.
	Int2 getInterfaceCenter(Game &game);

	// Helper functions for various UI textures.
	// @todo: these should probably return TextureAssetReferences to be like the other MVC files
	TextureBuilderID getGameWorldInterfaceTextureBuilderID(TextureManager &textureManager);
	TextureBuilderID getCompassFrameTextureBuilderID(Game &game);
	TextureBuilderID getCompassSliderTextureBuilderID(Game &game);
	TextureBuilderID getPlayerPortraitTextureBuilderID(Game &game, const std::string &portraitsFilename, int portraitID);
	TextureBuilderID getStatusGradientTextureBuilderID(Game &game, int gradientID);
	TextureBuilderID getNoSpellTextureBuilderID(Game &game);
	TextureBuilderID getWeaponTextureBuilderID(Game &game, const std::string &weaponFilename, int index);
}

#endif
