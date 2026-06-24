#include "CharacterEquipmentUiState.h"
#include "CharacterSheetUiMVC.h"
#include "CharacterUiState.h"
#include "InventoryUiMVC.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Items/ItemLibrary.h"
#include "../UI/FontLibrary.h"

namespace
{
	constexpr char ElementName_InventoryListBox[] = "CharacterEquipmentInventoryListBox";
}

CharacterEquipmentUiState::CharacterEquipmentUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
}

void CharacterEquipmentUiState::init(Game &game)
{
	this->game = &game;
}

void CharacterEquipmentUI::create(Game &game)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(CharacterEquipmentUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const CharacterEquipmentPresentationState equipmentPresentationState = CharacterSheetUiView::getEquipmentPresentationState(game);

	UiElementInitInfo bodyImageElementInitInfo;
	bodyImageElementInitInfo.name = "CharacterEquipmentBodyImage";
	bodyImageElementInitInfo.position = equipmentPresentationState.bodyPosition;
	bodyImageElementInitInfo.drawOrder = 1;
	uiManager.createImage(bodyImageElementInitInfo, equipmentPresentationState.bodyTextureID, state.contextInstID, renderer);

	UiElementInitInfo pantsImageElementInitInfo;
	pantsImageElementInitInfo.name = "CharacterEquipmentPantsImage";
	pantsImageElementInitInfo.position = equipmentPresentationState.pantsPosition;
	pantsImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(pantsImageElementInitInfo, equipmentPresentationState.pantsTextureID, state.contextInstID, renderer);

	UiElementInitInfo headImageElementInitInfo;
	headImageElementInitInfo.name = "CharacterEquipmentHeadImage";
	headImageElementInitInfo.position = equipmentPresentationState.headPosition;
	headImageElementInitInfo.drawOrder = 3;
	uiManager.createImage(headImageElementInitInfo, equipmentPresentationState.headTextureID, state.contextInstID, renderer);

	UiElementInitInfo shirtImageElementInitInfo;
	shirtImageElementInitInfo.name = "CharacterEquipmentShirtImage";
	shirtImageElementInitInfo.position = equipmentPresentationState.shirtPosition;
	shirtImageElementInitInfo.drawOrder = 4;
	uiManager.createImage(shirtImageElementInitInfo, equipmentPresentationState.shirtTextureID, state.contextInstID, renderer);

	uiManager.addMouseScrollChangedListener(CharacterEquipmentUI::onMouseScrollChanged, CharacterEquipmentUI::ContextName, inputManager);

	const std::string playerNameText = CharacterSheetUiModel::getPlayerName(game);
	const UiElementInstanceID playerNameTextBoxElementInstID = uiManager.getElementByName("CharacterEquipmentNameTextBox");
	uiManager.setTextBoxText(playerNameTextBoxElementInstID, playerNameText.c_str());

	const std::string playerRaceText = CharacterSheetUiModel::getPlayerRaceName(game);
	const UiElementInstanceID playerRaceTextBoxElementInstID = uiManager.getElementByName("CharacterEquipmentRaceTextBox");
	uiManager.setTextBoxText(playerRaceTextBoxElementInstID, playerRaceText.c_str());

	const std::string playerClassText = CharacterSheetUiModel::getPlayerClassName(game);
	const UiElementInstanceID playerClassTextBoxElementInstID = uiManager.getElementByName("CharacterEquipmentClassTextBox");
	uiManager.setTextBoxText(playerClassTextBoxElementInstID, playerClassText.c_str());

	const std::string playerLevelText = CharacterSheetUiModel::getPlayerLevel(game);
	const UiElementInstanceID levelTextBoxElementInstID = uiManager.getElementByName("CharacterEquipmentLevelTextBox");
	uiManager.setTextBoxText(levelTextBoxElementInstID, playerLevelText.c_str());

	const UiElementInstanceID inventoryListBoxElementInstID = uiManager.getElementByName("CharacterEquipmentInventoryListBox");
	const Buffer<InventoryUiModel::ItemUiDefinition> itemUiDefs = InventoryUiModel::getPlayerInventoryItems(game);
	for (int i = 0; i < itemUiDefs.getCount(); i++)
	{
		const InventoryUiModel::ItemUiDefinition &itemUiDef = itemUiDefs.get(i);

		UiListBoxItem listBoxItem;
		listBoxItem.text = itemUiDef.text;
		listBoxItem.overrideColor = itemUiDef.color;
		listBoxItem.callback = [&game, &uiManager, inventoryListBoxElementInstID, i](MouseButtonType mouseButtonType)
		{
			if (mouseButtonType == MouseButtonType::Left)
			{
				Player &player = game.player;
				ItemInventory &playerInventory = player.inventory;
				ItemInstance &itemInst = playerInventory.getSlot(i);
				itemInst.isEquipped = !itemInst.isEquipped;

				// @todo actually enforce equipment rules like only 1 weapon at a time, etc.. Currently this will just pick the first equipped weapon in inventory

				const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
				const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);
				if (itemDef.type == ItemType::Weapon)
				{
					const ItemDefinitionID equippedItemDefID = player.getEquippedWeaponItemDefID();
					player.setWeaponAnimationFromItem(equippedItemDefID); // Resets to sheathed animation state.
				}

				const Color displayColor = InventoryUiView::getItemDisplayColor(itemInst, player);
				uiManager.setListBoxItemColorOverride(inventoryListBoxElementInstID, i, displayColor);
			}
			else if (mouseButtonType == MouseButtonType::Right)
			{
				DebugLogFormat("Not implemented: inspect item %d.", i);
			}
		};

		uiManager.insertBackListBoxItem(inventoryListBoxElementInstID, std::move(listBoxItem));
	}

	game.setCursorOverride(std::nullopt);

	inputManager.setInputActionMapActive(InputActionMapName::CharacterEquipment, true);
}

void CharacterEquipmentUI::destroy()
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	inputManager.setInputActionMapActive(InputActionMapName::CharacterEquipment, false);
}

void CharacterEquipmentUI::update(double dt)
{
	// Do nothing.
}

void CharacterEquipmentUI::onMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position)
{
	if (type == MouseWheelScrollType::Down)
	{
		CharacterEquipmentUI::onInventoryListBoxDownButtonSelected(MouseButtonType::Left);
	}
	else if (type == MouseWheelScrollType::Up)
	{
		CharacterEquipmentUI::onInventoryListBoxUpButtonSelected(MouseButtonType::Left);
	}
}

void CharacterEquipmentUI::onInventoryListBoxUpButtonSelected(MouseButtonType mouseButtonType)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID inventoryListBoxElementInstID = uiManager.getElementByName(ElementName_InventoryListBox);
	uiManager.scrollListBoxUp(inventoryListBoxElementInstID);
}

void CharacterEquipmentUI::onInventoryListBoxDownButtonSelected(MouseButtonType mouseButtonType)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID inventoryListBoxElementInstID = uiManager.getElementByName(ElementName_InventoryListBox);
	uiManager.scrollListBoxDown(inventoryListBoxElementInstID);
}

void CharacterEquipmentUI::onExitButtonSelected(MouseButtonType mouseButtonType)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	game.setNextContext(CharacterUI::ContextName);
}

void CharacterEquipmentUI::onSpellbookButtonSelected(MouseButtonType mouseButtonType)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	DebugLogError("Not implemented: spellbook");
	// @todo open character spellbook UI once that is made
}

void CharacterEquipmentUI::onDropButtonSelected(MouseButtonType mouseButtonType)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	DebugLogError("Not implemented: drop item");
	// @todo drop item if there's room
}

void CharacterEquipmentUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		CharacterEquipmentUI::onExitButtonSelected(MouseButtonType::Left);
	}
}
