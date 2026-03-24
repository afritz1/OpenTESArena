#include "CharacterEquipmentPanel.h"
#include "CharacterEquipmentUiState.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "CharacterPanel.h"
#include "InventoryUiModel.h"
#include "InventoryUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../UI/FontLibrary.h"

namespace
{
	constexpr char ElementName_InventoryListBox[] = "CharacterEquipmentInventoryListBox";
}

CharacterEquipmentUiState::CharacterEquipmentUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->bodyTextureID = -1;
	this->pantsTextureID = -1;
	this->headTextureID = -1;
	this->shirtTextureID = -1;
}

void CharacterEquipmentUiState::init(Game &game)
{
	this->game = &game;

	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	this->bodyTextureID = CharacterSheetUiView::allocBodyTexture(game);
	this->pantsTextureID = CharacterSheetUiView::allocPantsTexture(game);
	this->headTextureID = CharacterSheetUiView::allocHeadTexture(game);
	this->shirtTextureID = CharacterSheetUiView::allocShirtTexture(game);
}

void CharacterEquipmentUiState::freeTextures(Renderer &renderer)
{
	if (this->bodyTextureID >= 0)
	{
		renderer.freeUiTexture(this->bodyTextureID);
		this->bodyTextureID = -1;
	}

	if (this->pantsTextureID >= 0)
	{
		renderer.freeUiTexture(this->pantsTextureID);
		this->pantsTextureID = -1;
	}

	if (this->headTextureID >= 0)
	{
		renderer.freeUiTexture(this->headTextureID);
		this->headTextureID = -1;
	}

	if (this->shirtTextureID >= 0)
	{
		renderer.freeUiTexture(this->shirtTextureID);
		this->shirtTextureID = -1;
	}
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

	UiElementInitInfo bodyImageElementInitInfo;
	bodyImageElementInitInfo.name = "CharacterEquipmentBodyImage";
	bodyImageElementInitInfo.position = CharacterSheetUiView::getBodyOffset(game);
	bodyImageElementInitInfo.drawOrder = 1;
	uiManager.createImage(bodyImageElementInitInfo, state.bodyTextureID, state.contextInstID, renderer);

	UiElementInitInfo pantsImageElementInitInfo;
	pantsImageElementInitInfo.name = "CharacterEquipmentPantsImage";
	pantsImageElementInitInfo.position = CharacterSheetUiView::getPantsOffset(game);
	pantsImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(pantsImageElementInitInfo, state.pantsTextureID, state.contextInstID, renderer);

	UiElementInitInfo headImageElementInitInfo;
	headImageElementInitInfo.name = "CharacterEquipmentHeadImage";
	headImageElementInitInfo.position = CharacterSheetUiView::getHeadOffset(game);
	headImageElementInitInfo.drawOrder = 3;
	uiManager.createImage(headImageElementInitInfo, state.headTextureID, state.contextInstID, renderer);

	UiElementInitInfo shirtImageElementInitInfo;
	shirtImageElementInitInfo.name = "CharacterEquipmentShirtImage";
	shirtImageElementInitInfo.position = CharacterSheetUiView::getShirtOffset(game);
	shirtImageElementInitInfo.drawOrder = 4;
	uiManager.createImage(shirtImageElementInitInfo, state.shirtTextureID, state.contextInstID, renderer);

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
				ItemInventory &playerInventory = game.player.inventory;
				ItemInstance &itemInst = playerInventory.getSlot(i);
				itemInst.isEquipped = !itemInst.isEquipped;

				const Color &equipColor = InventoryUiView::getItemDisplayColor(itemInst);
				uiManager.setListBoxItemColorOverride(inventoryListBoxElementInstID, i, equipColor);
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

	state.freeTextures(renderer);

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
	game.setPanel<CharacterPanel>();
}

void CharacterEquipmentUI::onSpellbookButtonSelected(MouseButtonType mouseButtonType)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	DebugLogError("Not implemented: spellbook");
	// @todo open character spellbook panel once that is made
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
