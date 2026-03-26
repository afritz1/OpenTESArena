#ifndef GAME_WORLD_UI_MVC_H
#define GAME_WORLD_UI_MVC_H

#include <functional>
#include <optional>
#include <string>

#include "../Assets/TextureUtils.h"
#include "../Entities/EntityInstance.h"
#include "../Math/MathUtils.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/UiListBox.h"
#include "../UI/UiPivotType.h"
#include "../Utilities/Color.h"
#include "../Voxels/VoxelFrustumCullingChunk.h"
#include "../World/Coord.h"

#include "components/utilities/Span.h"

class Game;
class FontLibrary;
class ItemInventory;

enum class MapType;

struct ExeData;
struct Player;
struct Rect;
struct UiDrawCommandList;
struct Window;

namespace GameWorldUiModel
{
	enum class ButtonType
	{
		CharacterSheet,
		ToggleWeapon,
		Map,
		Steal,
		Status,
		Magic,
		Logbook,
		UseItem,
		Camp
	};

	constexpr int BUTTON_COUNT = 9;

	std::string getPlayerNameText(Game &game);
	std::string getStatusButtonText(Game &game);

	OriginalInt2 getOriginalPlayerPosition(const WorldDouble3 &playerPos, MapType mapType);
	OriginalInt2 getOriginalPlayerPositionArenaUnits(const WorldDouble3 &playerPos, MapType mapType);
	std::string getPlayerPositionText(Game &game);

	std::optional<ButtonType> getHoveredButtonType(Game &game);
	bool isButtonTooltipAllowed(ButtonType buttonType, Game &game);
	std::string getButtonTooltip(ButtonType buttonType);

	void setFreeLookActive(Game &game, bool active);

	// Converts a screen point to a 3D direction in the world.
	VoxelDouble3 screenToWorldRayDirection(Game &game, const Int2 &windowPoint);

	Radians getCompassAngle(const VoxelDouble2 &direction);

	// Modifies the values in the native cursor regions array so rectangles in
	// the current window correctly represent regions for different arrow cursors.
	void updateNativeCursorRegions(Span<Rect> nativeCursorRegions, int width, int height);

	std::string getEnemyInspectedMessage(const std::string &entityName, const ExeData &exeData);
	std::string getEnemyCorpseGoldMessage(int goldCount, const ExeData &exeData);
	std::string getEnemyCorpseEmptyInventoryMessage(const std::string &entityName, const ExeData &exeData);
	std::string getCitizenKillGoldMessage(int goldCount, const ExeData &exeData);

	std::string getLockDifficultyMessage(int lockLevel, const ExeData &exeData);
	std::string getKeyPickUpMessage(int keyID, const ExeData &exeData);
	std::string getDoorUnlockWithKeyMessage(int keyID, const ExeData &exeData);

	std::string getStaminaExhaustedMessage(bool isSwimming, bool isInterior, bool isNight, const ExeData &exeData);
}

struct DebugVoxelVisibilityQuadtreeState
{
	static constexpr int TEXTURE_COUNT = VoxelFrustumCullingChunk::TREE_LEVEL_COUNT;

	UiTextureID textureIDs[TEXTURE_COUNT];
	Int2 textureDimsList[TEXTURE_COUNT];
	int drawPositionYs[TEXTURE_COUNT];
	RenderElement2D renderElements[TEXTURE_COUNT];

	DebugVoxelVisibilityQuadtreeState();

	void populateCommandList(Game &game, UiDrawCommandList &commandList);
	void free(Renderer &renderer);
};

namespace GameWorldUiView
{
	constexpr int ArrowCursorRegionCount = 9;

	// Original arrow cursor rectangles for each part of the classic UI. Their components can be multiplied by
	// the ratio of the native and the original resolution so they're flexible with most resolutions.
	constexpr Rect CursorRegions[ArrowCursorRegionCount] =
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
	constexpr UiPivotType ArrowCursorPivotTypes[ArrowCursorRegionCount] =
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

	Rect getCharacterSheetButtonRect();
	Rect getPlayerPortraitRect();
	Rect getWeaponSheathButtonRect();
	Rect getMapButtonRect();
	Rect getStealButtonRect();
	Rect getStatusButtonRect();
	Rect getMagicButtonRect();
	Rect getLogbookButtonRect();
	Rect getUseItemButtonRect();
	Rect getCampButtonRect();
	Rect getScrollUpButtonRect();
	Rect getScrollDownButtonRect();
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

	UiListBoxInitInfo getLootListBoxProperties();

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

	UiTextureID allocStatusBarsTexture(TextureManager &textureManager, Renderer &renderer);
	int getStatusBarCurrentPixelHeight(double currentValue, double maxValue);
	void updateStatusBarsTexture(UiTextureID textureID, const Player &player, Renderer &renderer);

	UiTextureID allocStatusGradientTexture(StatusGradientType gradientType, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocPlayerPortraitTexture(bool isMale, int raceID, int portraitID, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocWeaponAnimTexture(const std::string &weaponFilename, int index, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocTooltipTexture(GameWorldUiModel::ButtonType buttonType, const FontLibrary &fontLibrary, Renderer &renderer);
	UiTextureID allocArrowCursorTexture(int cursorIndex, TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocModernModeReticleTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocKeyTexture(int keyIndex, TextureManager &textureManager, Renderer &renderer);

	void DEBUG_ColorRaycastPixel(Game &game);
	void DEBUG_PhysicsRaycast(Game &game);

	DebugVoxelVisibilityQuadtreeState allocDebugVoxelVisibilityQuadtreeState(Renderer &renderer);
}

namespace GameWorldUiController
{
	void onStatusPopUpSelected(Game &game);

	void onEnemyAliveInspected(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef);
	void onContainerInventoryOpened(Game &game, EntityInstanceID entityInstID, ItemInventory &itemInventory, bool destroyEntityIfEmpty);
	void onEnemyCorpseInteracted(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef);
	void onEnemyCorpseInteractedFirstTime(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef);
	void onEnemyCorpseEmptyInventoryOpened(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef);

	void onKeyPickedUp(Game &game, int keyID, const ExeData &exeData, const std::function<void()> &postStatusPopUpCallback);
	void onDoorUnlockedWithKey(Game &game, int keyID, const std::string &soundFilename, const WorldDouble3 &soundPosition, const ExeData &exeData);

	void onCitizenInteracted(Game &game, const EntityInstance &entityInst);
	void onCitizenKilled(Game &game);

	void onStaticNpcInteracted(Game &game, StaticNpcPersonalityType personalityType);

	void onShowPlayerDeathCinematic(Game &game);
	void onHealthDepleted(Game &game);
	void onStaminaExhausted(Game &game, bool isSwimming, bool isInterior, bool isNight);
}

#endif
