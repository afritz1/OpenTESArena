#ifndef GAME_WORLD_UI_VIEW_H
#define GAME_WORLD_UI_VIEW_H

#include <array>

#include "GameWorldUiModel.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/ArenaFontName.h"
#include "../UI/ListBox.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/UiPivotType.h"
#include "../Utilities/Color.h"
#include "../Voxels/VoxelFrustumCullingChunk.h"

class FontLibrary;
class Game;

struct UiCommandList;
struct Window;

struct DebugVoxelVisibilityQuadtreeState
{
	static constexpr int TEXTURE_COUNT = VoxelFrustumCullingChunk::TREE_LEVEL_COUNT;

	UiTextureID textureIDs[TEXTURE_COUNT];
	Int2 textureDimsList[TEXTURE_COUNT];
	int drawPositionYs[TEXTURE_COUNT];
	RenderElement2D renderElements[TEXTURE_COUNT];

	DebugVoxelVisibilityQuadtreeState();

	void populateCommandList(Game &game, UiCommandList &commandList);
	void free(Renderer &renderer);
};

namespace GameWorldUiView
{
	constexpr int ArrowCursorRegionCount = 9;

	// Original arrow cursor rectangles for each part of the classic UI. Their components can be multiplied by
	// the ratio of the native and the original resolution so they're flexible with most resolutions.
	const std::array<Rect, ArrowCursorRegionCount> CursorRegions =
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
	constexpr Rect UiBottomRegion(0, ArenaRenderUtils::SCREEN_HEIGHT - ArenaRenderUtils::SCENE_UI_HEIGHT, ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCENE_UI_HEIGHT);

	// Arrow cursor pivots. These offset the drawn cursor relative to the mouse position so the cursor's
	// click area is closer to the tip of each arrow.
	constexpr std::array<UiPivotType, ArrowCursorRegionCount> ArrowCursorPivotTypes =
	{
		UiPivotType::TopLeft,
		UiPivotType::Top,
		UiPivotType::TopRight,
		UiPivotType::BottomLeft,
		UiPivotType::Middle,
		UiPivotType::BottomRight,
		UiPivotType::MiddleLeft,
		UiPivotType::Bottom,
		UiPivotType::MiddleRight,
	};

	enum class StatusGradientType
	{
		Default
	};

	constexpr int PlayerNameTextBoxX = 17;
	constexpr int PlayerNameTextBoxY = 154;
	const std::string PlayerNameFontName = ArenaFontName::Char;
	const Color PlayerNameTextColor(215, 121, 8);
	constexpr TextAlignment PlayerNameTextAlignment = TextAlignment::TopLeft;

	TextBoxInitInfo getPlayerNameTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary);

	Rect getCharacterSheetButtonRect();
	Rect getPlayerPortraitRect();
	Rect getWeaponSheathButtonRect();
	Rect getStealButtonRect();
	Rect getStatusButtonRect();
	Rect getMagicButtonRect();
	Rect getLogbookButtonRect();
	Rect getUseItemButtonRect();
	Rect getCampButtonRect();
	Rect getScrollUpButtonRect();
	Rect getScrollDownButtonRect();
	Rect getMapButtonRect();
	Rect getButtonRect(GameWorldUiModel::ButtonType buttonType);
	
	const std::string StatusPopUpFontName = ArenaFontName::Arena;
	const Color StatusPopUpTextColor(251, 239, 77);
	constexpr TextAlignment StatusPopUpTextAlignment = TextAlignment::MiddleCenter;
	constexpr int StatusPopUpTextLineSpacing = 1;

	constexpr UiTexturePatternType StatusPopUpTexturePatternType = UiTexturePatternType::Dark;

	Int2 getStatusPopUpTextCenterPoint(Game &game);
	int getStatusPopUpTextureWidth(int textWidth);
	int getStatusPopUpTextureHeight(int textHeight);

	Int2 getGameWorldInterfacePosition();

	constexpr Rect HealthBarRect(57, 168, 4, 26);
	constexpr Color HealthBarColor(0, 182, 0);
	constexpr Rect StaminaBarRect(67, 168, 4, 26);
	constexpr Color StaminaBarColor(195, 0, 0);
	constexpr Rect SpellPointsBarRect(77, 168, 4, 26);
	constexpr Color SpellPointsBarColor(0, 0, 203);
	constexpr UiPivotType StatusBarPivotType = UiPivotType::BottomLeft;

	// Offsets from native window bottom left corner.
	constexpr int StatusBarModernModeXOffset = 48;
	constexpr int StatusBarModernModeYOffset = 32;

	int getStatusBarCurrentHeight(int maxHeight, double currentValue, double maxValue);

	Int2 getNoMagicTexturePosition();

	int getKeyTextureCount(TextureManager &textureManager);
	Int2 getKeyPosition(int keyIndex);

	const std::string TriggerTextFontName = ArenaFontName::Arena;
	const Color TriggerTextColor(215, 121, 8);
	constexpr TextAlignment TriggerTextAlignment = TextAlignment::BottomCenter;
	constexpr int TriggerTextShadowOffsetX = -1;
	constexpr int TriggerTextShadowOffsetY = 0;
	const Color TriggerTextShadowColor(12, 12, 24);
	constexpr int TriggerTextLineSpacing = 1;

	const std::string ActionTextFontName = ArenaFontName::Arena;
	const Color ActionTextColor(195, 0, 0);
	constexpr TextAlignment ActionTextAlignment = TextAlignment::TopCenter;
	constexpr int ActionTextShadowOffsetX = -1;
	constexpr int ActionTextShadowOffsetY = 0;
	const Color ActionTextShadowColor(12, 12, 24);

	const Color EffectTextColor(251, 239, 77);
	const Color EffectTextShadowColor(190, 113, 0);

	Int2 getTriggerTextPosition(Game &game, int gameWorldInterfaceTextureHeight);
	Int2 getActionTextPosition();
	Int2 getEffectTextPosition();

	double getTriggerTextSeconds(const std::string_view text);
	double getActionTextSeconds(const std::string_view text);
	double getEffectTextSeconds(const std::string_view text);

	TextBoxInitInfo getTriggerTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getActionTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getEffectTextBoxInitInfo(const FontLibrary &fontLibrary);

	ListBoxProperties getLootListBoxProperties();

	Int2 getTooltipPosition(Game &game);

	Rect getCompassClipRect();
	Int2 getCompassSliderPosition(Game &game, const VoxelDouble2 &playerDirection);
	Int2 getCompassFramePosition();

	Int2 getWeaponAnimationOffset(const std::string &weaponFilename, int frameIndex, TextureManager &textureManager);

	// Gets the center of the screen for pop-up related functions. The position depends on
	// whether modern interface mode is set.
	Int2 getInterfaceCenter(Game &game);

	// Gets the pixel coordinate of the center of the window. Does not handle classic vs. modern mode.
	Int2 getNativeWindowCenter(const Window &window);

	// Helper functions for various UI textures.
	TextureAsset getPaletteTextureAsset();
	TextureAsset getGameWorldInterfaceTextureAsset();
	TextureAsset getStatusGradientTextureAsset(StatusGradientType gradientType);
	TextureAsset getPlayerPortraitTextureAsset(bool isMale, int raceID, int portraitID);
	TextureAsset getNoMagicTextureAsset();
	TextureAsset getWeaponAnimTextureAsset(const std::string &weaponFilename, int index);
	TextureAsset getCompassFrameTextureAsset();
	TextureAsset getCompassSliderTextureAsset();
	TextureAsset getArrowCursorTextureAsset(int cursorIndex);
	TextureAsset getKeyTextureAsset(int keyIndex);
	TextureAsset getContainerInventoryTextureAsset();

	UiTextureID allocGameWorldInterfaceTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocHealthBarTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocStaminaBarTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocSpellPointsBarTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocStatusGradientTexture(StatusGradientType gradientType, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocPlayerPortraitTexture(bool isMale, int raceID, int portraitID, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocNoMagicTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocWeaponAnimTexture(const std::string &weaponFilename, int index, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocCompassFrameTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocCompassSliderTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocTooltipTexture(GameWorldUiModel::ButtonType buttonType, const FontLibrary &fontLibrary, Renderer &renderer);
	UiTextureID allocArrowCursorTexture(int cursorIndex, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocModernModeReticleTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocKeyTexture(int keyIndex, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocContainerInventoryTexture(TextureManager &textureManager, Renderer &renderer);

	void DEBUG_ColorRaycastPixel(Game &game);
	void DEBUG_PhysicsRaycast(Game &game);

	DebugVoxelVisibilityQuadtreeState allocDebugVoxelVisibilityQuadtreeState(Renderer &renderer);
}

#endif
