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
#include "../UI/TextBox.h"

class Game;

namespace GameWorldUiView
{
	// Original arrow cursor rectangles for each part of the classic UI. Their components can be multiplied by
	// the ratio of the native and the original resolution so they're flexible with most resolutions.
	const std::array<Rect, 9> CursorRegions =
	{
		Rect(0, 0, 141, 49),
		Rect(141, 0, 38, 49),
		Rect(179, 0, 141, 49),
		Rect(0, 49, 90, 70),
		Rect(90, 49, 140, 70),
		Rect(230, 49, 90, 70),
		Rect(0, 119, 141, 28),
		Rect(141, 119, 38, 28),
		Rect(179, 119, 141, 28),
	};

	// Arrow cursor rectangle array indices.
	constexpr int CursorTopLeftIndex = 0;
	constexpr int CursorTopMiddleIndex = 1;
	constexpr int CursorTopRightIndex = 2;
	constexpr int CursorMiddleLeftIndex = 3;
	constexpr int CursorMiddleIndex = 4;
	constexpr int CursorMiddleRightIndex = 5;
	constexpr int CursorBottomLeftIndex = 6;
	constexpr int CursorBottomMiddleIndex = 7;
	constexpr int CursorBottomRightIndex = 8;

	// Makes a cursor region that scales to the current resolution.
	Rect scaleClassicCursorRectToNative(int rectIndex, double xScale, double yScale);

	// Game world interface UI area.
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

	TextBox::InitInfo getPlayerNameTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	constexpr int CharacterSheetButtonX = 14;
	constexpr int CharacterSheetButtonY = 166;
	constexpr int CharacterSheetButtonWidth = 40;
	constexpr int CharacterSheetButtonHeight = 29;

	constexpr int PlayerPortraitX = 14;
	constexpr int PlayerPortraitY = 166;

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

	Int2 getGameWorldInterfacePosition(int textureHeight);

	Int2 getNoMagicTexturePosition();

	constexpr FontName TriggerTextFontName = FontName::Arena;
	const Color TriggerTextColor(215, 121, 8);
	constexpr TextAlignment TriggerTextAlignment = TextAlignment::Center;
	constexpr int TriggerTextShadowOffsetX = -1;
	constexpr int TriggerTextShadowOffsetY = 0;
	const Color TriggerTextShadowColor(12, 12, 24);
	constexpr int TriggerTextLineSpacing = 1;

	constexpr FontName ActionTextFontName = FontName::Arena;
	const Color ActionTextColor(195, 0, 0);
	constexpr TextAlignment ActionTextAlignment = TextAlignment::Center;
	constexpr int ActionTextShadowOffsetX = -1;
	constexpr int ActionTextShadowOffsetY = 0;
	const Color ActionTextShadowColor(12, 12, 24);

	const Color EffectTextColor(251, 239, 77);
	const Color EffectTextShadowColor(190, 113, 0);

	Int2 getTriggerTextPosition(Game &game, int textWidth, int textHeight, int gameWorldInterfaceTextureHeight);
	Int2 getActionTextPosition(int textWidth);
	Int2 getEffectTextPosition();

	double getTriggerTextSeconds(const std::string_view &text);
	double getActionTextSeconds(const std::string_view &text);
	double getEffectTextSeconds(const std::string_view &text);

	TextBox::InitInfo getTriggerTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBox::InitInfo getActionTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBox::InitInfo getEffectTextBoxInitInfo(const FontLibrary &fontLibrary);

	Int2 getTooltipPosition(Game &game, int textureHeight);

	Rect getCompassClipRect(Game &game, const VoxelDouble2 &playerDirection, int textureHeight);
	Int2 getCompassSliderPosition(int clipWidth, int clipHeight);
	Int2 getCompassFramePosition(int textureWidth);
	TextureAssetReference getCompassSliderPaletteTextureAssetRef();

	Int2 getCurrentWeaponAnimationOffset(Game &game);
	std::optional<TextureBuilderID> getActiveWeaponAnimationTextureBuilderID(Game &game);

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

	TextureAssetReference getDefaultPaletteTextureAssetRef();

	void DEBUG_ColorRaycastPixel(Game &game);
	void DEBUG_PhysicsRaycast(Game &game);
	void DEBUG_DrawProfiler(Game &game, Renderer &renderer);
}

#endif
