#ifndef GAME_WORLD_UI_STATE_H
#define GAME_WORLD_UI_STATE_H

#include <functional>
#include <vector>

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

#include "components/utilities/Buffer.h"

class Game;
class ItemInventory;

using GameWorldPopUpClosedCallback = std::function<void()>;

// For keeping loot list box callbacks valid when removing inventory items.
struct GameWorldLootUiItemMapping
{
	int inventoryItemIndex;
	int listBoxItemIndex;

	GameWorldLootUiItemMapping();
};

struct GameWorldUiState
{
	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID textPopUpContextInstID;
	UiContextInstanceID lootPopUpContextInstID;

	UiTextureID statusBarsTextureID; // Health + stamina + spell points.
	UiTextureID statusGradientTextureID;
	UiTextureID playerPortraitTextureID;
	Buffer<UiTextureID> weaponAnimTextureIDs;
	Buffer<UiTextureID> keyTextureIDs;
	Buffer<UiTextureID> arrowCursorTextureIDs;
	UiTextureID modernModeReticleTextureID;

	double currentHealth;
	double maxHealth;
	double currentStamina;
	double maxStamina;
	double currentSpellPoints;
	double maxSpellPoints;

	std::vector<GameWorldLootUiItemMapping> lootPopUpItemMappings;

	GameWorldUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace GameWorldUI
{
	DECLARE_UI_CONTEXT(GameWorld);

	void onScreenToWorldInteraction(Int2 windowPoint, bool isPrimaryInteraction);
	void updateDoorKeys();
	void onPauseChanged(bool paused);

	void showTextPopUp(const char *str, const GameWorldPopUpClosedCallback &callback);
	void showTextPopUp(const char *str);
	void showLootPopUp(ItemInventory &itemInventory, const GameWorldPopUpClosedCallback &callback);

	void setTriggerText(const char *str);
	void setActionText(const char *str);

	void onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed);
	void onMouseButtonHeld(Game &game, MouseButtonType type, const Int2 &position, double dt);

	void onCharacterSheetButtonSelected(MouseButtonType mouseButtonType);
	void onWeaponToggleButtonSelected(MouseButtonType mouseButtonType);
	void onMapButtonSelected(MouseButtonType mouseButtonType);
	void onStealButtonSelected(MouseButtonType mouseButtonType);
	void onStatusButtonSelected(MouseButtonType mouseButtonType);
	void onMagicButtonSelected(MouseButtonType mouseButtonType);
	void onLogbookButtonSelected(MouseButtonType mouseButtonType);
	void onUseItemButtonSelected(MouseButtonType mouseButtonType);
	void onCampButtonSelected(MouseButtonType mouseButtonType);
	void onScrollUpButtonSelected(MouseButtonType mouseButtonType);
	void onScrollDownButtonSelected(MouseButtonType mouseButtonType);

	void onActivateInputAction(const InputActionCallbackValues &values);
	void onInspectInputAction(const InputActionCallbackValues &values);
	void onCharacterSheetInputAction(const InputActionCallbackValues &values);
	void onToggleWeaponInputAction(const InputActionCallbackValues &values);
	void onAutomapInputAction(const InputActionCallbackValues &values);
	void onWorldMapInputAction(const InputActionCallbackValues &values);
	void onStealInputAction(const InputActionCallbackValues &values);
	void onStatusInputAction(const InputActionCallbackValues &values);
	void onCastMagicInputAction(const InputActionCallbackValues &values);
	void onLogbookInputAction(const InputActionCallbackValues &values);
	void onUseItemInputAction(const InputActionCallbackValues &values);
	void onCampInputAction(const InputActionCallbackValues &values);
	void onToggleCompassInputAction(const InputActionCallbackValues &values);
	void onPlayerPositionInputAction(const InputActionCallbackValues &values);
	void onPauseMenuInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(GameWorldUI, onCharacterSheetButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onWeaponToggleButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onMapButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onStealButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onStatusButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onMagicButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onLogbookButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onUseItemButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onCampButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onScrollUpButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onScrollDownButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(GameWorldUI, onActivateInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onInspectInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onCharacterSheetInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onToggleWeaponInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onAutomapInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onWorldMapInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onStealInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onStatusInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onCastMagicInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onLogbookInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onUseItemInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onCampInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onToggleCompassInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onPlayerPositionInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onPauseMenuInputAction)
	};
}

#endif
