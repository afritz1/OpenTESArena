#pragma once

#include <functional>
#include <vector>

#include "DialogueManager.h"
#include "../Entities/EntityInstance.h"
#include "../Items/ItemLibrary.h"
#include "../Math/Rect.h"
#include "../Player/Player.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiButton.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

#include "components/utilities/Buffer.h"

class Game;
class ItemInventory;

using GameWorldPopUpClosedCallback = std::function<void()>;

enum class GameWorldInteractionType
{
	Default,
	Thieving
};

enum class ConversationMessageBoxType
{
	Citizen,
	CitizenRumors,
	Equipment,
	EquipmentBuyItem,
	MagesGuild,
	MagesGuildBuyItem,
	MagesGuildSteal,
	Tavern,
	TavernRumors,
	Temple
};

enum class ConversationListBoxType
{
	CitizenWhereIs,
	EquipmentWeapons,
	EquipmentArmor,
	EquipmentSell,
	EquipmentRepair,
	MagesGuildPotions,
	MagesGuildMagicItems,
	MagesGuildSpells,
	TavernDrinks,
	TavernRooms,
	TempleCuring
};

// For keeping loot list box callbacks valid when removing inventory items.
struct GameWorldLootUiItemMapping
{
	int inventoryItemIndex;
	int listBoxItemIndex;

	GameWorldLootUiItemMapping();
};

struct GameWorldUiInitInfo
{
	std::string textPopUpMessage; // For city arrival.

	void init(const std::string &textPopUpMessage);
};

struct GameWorldUiState
{
	GameWorldUiInitInfo initInfo;

	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID textPopUpContextInstID;
	UiContextInstanceID lootPopUpContextInstID;
	UiContextInstanceID campModalContextInstID;
	UiContextInstanceID campManualHoursModalContextInstID;
	UiContextInstanceID conversationModalContextInstID;
	UiContextInstanceID shopkeeperBgContextInstID; // Just decoration, no interaction.

	UiTextureID statusBarsTextureID; // Health + stamina + spell points.
	Buffer<UiTextureID> keyTextureIDs;
	Buffer<UiTextureID> arrowCursorTextureIDs;
	UiTextureID playerHurtTextureID;
	UiTextureID modernModeReticleTextureID;

	// Screen regions for classic interface movement in the game world, scaled to fit the current window.
	Rect nativeCursorRegions[9];

	double currentHealth;
	double maxHealth;
	double currentStamina;
	double maxStamina;
	double currentSpellPoints;
	double maxSpellPoints;

	GameWorldInteractionType interactionType;

	// Game world interface display texts have an associated time remaining. These values are not destroyed when switching away from the game world UI.
	// Camping hours text doesn't have a duration, it's only when camping is active.
	// - Trigger text: lore message from voxel trigger
	// - Action text: description of the player's current action
	// - Effect text: effect on the player (disease, drunk, silence, etc.)
	double triggerTextRemainingSeconds, actionTextRemainingSeconds, effectTextRemainingSeconds;

	double playerHurtRemainingSeconds;

	std::vector<GameWorldLootUiItemMapping> lootPopUpItemMappings;

	std::string campManualHoursInputText; // Number of hours to manually rest.

	std::vector<DialogueDirectionsDetailEntry> dialogueWhereIsDetailEntries; // After asking for all inns/temples/stores in "Where Is..." menu.
	PlayerEffectsState dialogueStartPlayerEffectsState; // Cached when beginning dialogue and checked upon leaving dialogue (for drunk effect).

	GameWorldUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);

	void updateNativeCursorRegions(int windowWidth, int windowHeight);
};

namespace GameWorldUI
{
	DECLARE_UI_CONTEXT(GameWorld);

	void onScreenToWorldInteraction(Int2 windowPoint, bool isPrimaryInteraction);
	void updateDoorKeys();
	void setCompassVisible(bool visible);
	void setInteractionType(GameWorldInteractionType type);
	void onPauseChanged(bool paused);

	void showTextPopUp(const char *str, const std::string &fontName, TextAlignment alignment, const GameWorldPopUpClosedCallback &callback);
	void showTextPopUp(const char *str, const std::string &fontName, TextAlignment alignment);
	void showLootPopUp(ItemInventory &itemInventory, const GameWorldPopUpClosedCallback &callback);
	void showCampModal();
	void showCampManualHoursModal();
	void showPlayerHurt();

	// Sets only the given map active and other conversation maps inactive.
	void setConversationMessageBoxInputActionMapActive(const char *mapName);
	void addConversationMessageBoxInputActionListeners(const char *mapName);

	void showConversationMessageBox(ConversationMessageBoxType messageBoxType);
	void showConversationListBox(ConversationListBoxType listBoxType);
	void showShopkeeperBackground(const char *titleText);
	GameWorldPopUpClosedCallback makeReturnToMessageBoxCallback(ConversationMessageBoxType messageBoxType);
	void setShopkeeperPlayerGoldVisible(bool visible);
	UiButtonCallback makeDirectionsEntryCallback(const DialogueDirectionsEntry &entry);
	UiButtonCallback makeDirectionsDetailEntryCallback(const DialogueDirectionsDetailEntry &detailEntry);
	UiButtonCallback makeShopkeeperItemPurchaseCallback(ItemDefinitionID itemDefID, int itemGoldPrice, ConversationMessageBoxType returnMessageBoxType);
	UiButtonCallback makeShopkeeperItemSellCallback(int playerInventorySlotIndex);
	UiButtonCallback makeShopkeeperItemRepairCallback(int playerInventorySlotIndex);
	UiButtonCallback makeMagesGuildSpellPurchaseCallback(const std::string &spellName);
	UiButtonCallback makeTavernDrinkPurchaseCallback(const std::string &drinkName);
	UiButtonCallback makeTavernRoomPurchaseCallback(int roomType);
	UiButtonCallback makeTempleCurePurchaseCallback();
	void onPlayerStealItemSuccess(const ItemLibraryPredicate &stealableItemsPredicate, ConversationMessageBoxType mainMessageBoxType);
	void onPlayerStealItemFailure();

	bool isTriggerTextVisible();
	bool isActionTextVisible();
	bool isEffectTextVisible();
	bool isCampingHoursTextVisible();
	void setTriggerText(const char *str);
	void setActionText(const char *str);
	void setEffectText(const char *str);
	void setCampingHoursText(const char *str);
	void setTriggerTextDuration(const std::string_view text);
	void setActionTextDuration(const std::string_view text);
	void setEffectTextDuration(const std::string_view text);
	void resetTriggerTextDuration();
	void resetActionTextDuration();
	void resetEffectTextDuration();

	void onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed);
	void onMouseButtonHeld(Game &game, MouseButtonType type, const Int2 &position, double dt);
	void onWindowResized(int width, int height);

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

	void onCloseConversationButtonSelected(MouseButtonType mouseButtonType);
	void onNpcWhoAreYouButtonSelected(MouseButtonType mouseButtonType);
	void onNpcWhereIsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcRumorsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcRumorsGeneralButtonSelected(MouseButtonType mouseButtonType);
	void onNpcRumorsWorkButtonSelected(MouseButtonType mouseButtonType);
	void onNpcEquipmentBuyButtonSelected(MouseButtonType mouseButtonType);
	void onNpcEquipmentSellButtonSelected(MouseButtonType mouseButtonType);
	void onNpcEquipmentRepairButtonSelected(MouseButtonType mouseButtonType);
	void onNpcEquipmentStealButtonSelected(MouseButtonType mouseButtonType);
	void onNpcEquipmentBuyWeaponsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcEquipmentBuyArmorButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildBuyButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildDetectMagicButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildSpellmakerButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildStealButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildBuyPotionsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildBuyMagicItemsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildBuySpellsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildStealPotionsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcMagesGuildStealMagicItemsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTavernBuyDrinksButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTavernGetARoomButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTavernSneakIntoARoomButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTavernRumorsButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTavernRumorsGeneralButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTavernRumorsWorkButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTempleBlessButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTempleCureButtonSelected(MouseButtonType mouseButtonType);
	void onNpcTempleHealButtonSelected(MouseButtonType mouseButtonType);

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

	void onNpcWhoAreYouInputAction(const InputActionCallbackValues &values);
	void onNpcWhereIsInputAction(const InputActionCallbackValues &values);
	void onNpcRumorsInputAction(const InputActionCallbackValues &values);
	void onNpcExitInputAction(const InputActionCallbackValues &values);
	void onNpcRumorsGeneralInputAction(const InputActionCallbackValues &values);
	void onNpcRumorsWorkInputAction(const InputActionCallbackValues &values);
	void onEquipmentStoreBuyInputAction(const InputActionCallbackValues &values);
	void onEquipmentStoreSellInputAction(const InputActionCallbackValues &values);
	void onEquipmentStoreRepairInputAction(const InputActionCallbackValues &values);
	void onEquipmentStoreStealInputAction(const InputActionCallbackValues &values);
	void onEquipmentStoreExitInputAction(const InputActionCallbackValues &values);
	void onEquipmentStoreBuyWeaponInputAction(const InputActionCallbackValues &values);
	void onEquipmentStoreBuyArmorInputAction(const InputActionCallbackValues &values);
	void onMagesGuildBuyInputAction(const InputActionCallbackValues &values);
	void onMagesGuildDetectMagicInputAction(const InputActionCallbackValues &values);
	void onMagesGuildSpellmakerInputAction(const InputActionCallbackValues &values);
	void onMagesGuildStealInputAction(const InputActionCallbackValues &values);
	void onMagesGuildExitInputAction(const InputActionCallbackValues &values);
	void onMagesGuildBuyPotionsInputAction(const InputActionCallbackValues &values);
	void onMagesGuildBuyMagicItemsInputAction(const InputActionCallbackValues &values);
	void onMagesGuildBuySpellsInputAction(const InputActionCallbackValues &values);
	void onMagesGuildStealPotionsInputAction(const InputActionCallbackValues &values);
	void onMagesGuildStealMagicItemsInputAction(const InputActionCallbackValues &values);
	void onTavernBuyDrinksInputAction(const InputActionCallbackValues &values);
	void onTavernGetRoomInputAction(const InputActionCallbackValues &values);
	void onTavernSneakIntoRoomInputAction(const InputActionCallbackValues &values);
	void onTavernRumorsInputAction(const InputActionCallbackValues &values);
	void onTavernExitInputAction(const InputActionCallbackValues &values);
	void onTavernRumorsGeneralInputAction(const InputActionCallbackValues &values);
	void onTavernRumorsWorkInputAction(const InputActionCallbackValues &values);
	void onTempleBlessInputAction(const InputActionCallbackValues &values);
	void onTempleCureInputAction(const InputActionCallbackValues &values);
	void onTempleHealInputAction(const InputActionCallbackValues &values);
	void onTempleExitInputAction(const InputActionCallbackValues &values);

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
		DECLARE_UI_FUNC(GameWorldUI, onScrollDownButtonSelected),

		DECLARE_UI_FUNC(GameWorldUI, onCloseConversationButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcWhoAreYouButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcWhereIsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcRumorsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcRumorsGeneralButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcRumorsWorkButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcEquipmentBuyButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcEquipmentSellButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcEquipmentRepairButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcEquipmentStealButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcEquipmentBuyWeaponsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcEquipmentBuyArmorButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildBuyButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildDetectMagicButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildSpellmakerButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildStealButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildBuyPotionsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildBuyMagicItemsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildBuySpellsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildStealPotionsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcMagesGuildStealMagicItemsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTavernBuyDrinksButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTavernGetARoomButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTavernSneakIntoARoomButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTavernRumorsButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTavernRumorsGeneralButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTavernRumorsWorkButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTempleBlessButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTempleCureButtonSelected),
		DECLARE_UI_FUNC(GameWorldUI, onNpcTempleHealButtonSelected)
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
		DECLARE_UI_FUNC(GameWorldUI, onPauseMenuInputAction),

		DECLARE_UI_FUNC(GameWorldUI, onNpcWhoAreYouInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onNpcWhereIsInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onNpcRumorsInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onNpcExitInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onNpcRumorsGeneralInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onNpcRumorsWorkInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onEquipmentStoreBuyInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onEquipmentStoreSellInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onEquipmentStoreRepairInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onEquipmentStoreStealInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onEquipmentStoreExitInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onEquipmentStoreBuyWeaponInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onEquipmentStoreBuyArmorInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildBuyInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildDetectMagicInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildSpellmakerInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildStealInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildExitInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildBuyPotionsInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildBuyMagicItemsInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildBuySpellsInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildStealPotionsInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onMagesGuildStealMagicItemsInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTavernBuyDrinksInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTavernGetRoomInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTavernSneakIntoRoomInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTavernRumorsInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTavernExitInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTavernRumorsGeneralInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTavernRumorsWorkInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTempleBlessInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTempleCureInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTempleHealInputAction),
		DECLARE_UI_FUNC(GameWorldUI, onTempleExitInputAction)
	};
}
