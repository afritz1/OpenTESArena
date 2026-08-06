#include "CharacterEquipmentUiState.h"
#include "CharacterSheetUiMVC.h"
#include "CharacterUiState.h"
#include "InventoryUiMVC.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Items/ItemLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontLibrary.h"

#include "components/utilities/String.h"

namespace
{
	constexpr char ContextName_ItemDetail[] = "CharacterEquipmentItemDetail";

	constexpr char ElementName_InventoryListBox[] = "CharacterEquipmentInventoryListBox";	
	constexpr char ElementName_InventoryListBoxHighlight[] = "CharacterEquipmentInventoryListBoxHighlight";

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;

	UiListBoxItemCallback MakeInventoryListBoxItemCallback(Game &game, UiElementInstanceID listBoxElementInstID, int listBoxItemIndex)
	{
		return [&game, listBoxElementInstID, listBoxItemIndex](MouseButtonType mouseButtonType)
		{
			UiManager &uiManager = game.uiManager;
			const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

			Player &player = game.player;
			ItemInventory &playerInventory = player.inventory;
			const CharacterClassDefinition &charClassDef = CharacterClassLibrary::getInstance().getDefinition(player.charClassDefID);

			const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
			ItemInstance &itemInst = playerInventory.getSlot(listBoxItemIndex);
			const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);
			const ItemType itemType = itemDef.type;

			if (mouseButtonType == MouseButtonType::Left)
			{
				if (!itemDef.isEquippable)
				{
					std::string unequippableItemStr = exeData.equipment.unequippableItem;
					unequippableItemStr.pop_back(); // Clean up newline
					CharacterEquipmentUI::showItemDetail(unequippableItemStr.c_str(), CharacterEquipmentUiView::ItemDetailErrorTextColor);
					return;
				}

				const bool isEquippableByClass = InventoryUiModel::isItemEquippableByClass(itemDef, charClassDef);
				if (!isEquippableByClass)
				{
					std::string forbiddenByClassStr = String::replace(exeData.equipment.classForbiddenItem, "%s", charClassDef.name);
					forbiddenByClassStr.pop_back(); // Clean up newline
					CharacterEquipmentUI::showItemDetail(forbiddenByClassStr.c_str(), CharacterEquipmentUiView::ItemDetailErrorTextColor);
					return;
				}

				bool isSimilarItemAlreadyEquipped = false; // Only matters for jewelry
				std::string similarItemBaseName;
				for (int i = 0; i < playerInventory.getTotalSlotCount(); i++)
				{
					if (i == listBoxItemIndex)
					{
						continue;
					}

					ItemInstance &currentItemInst = playerInventory.getSlot(i);
					if (currentItemInst.isValid() && currentItemInst.isEquipped)
					{
						const ItemDefinition &currentItemDef = itemLibrary.getDefinition(currentItemInst.defID);
						const ItemType currentItemType = currentItemDef.type;
						if (currentItemType == itemDef.type)
						{
							if (currentItemType == ItemType::Accessory)
							{
								const ArenaAccessoryTypeID currentAccessoryTypeID = currentItemDef.accessory.typeID;
								if (currentAccessoryTypeID == itemDef.accessory.typeID)
								{
									DebugAssertIndex(exeData.equipment.enhancementItemNames, currentAccessoryTypeID);
									similarItemBaseName = exeData.equipment.enhancementItemNames[currentAccessoryTypeID];
								}
							}
							else if (currentItemType == ItemType::Trinket)
							{
								const ArenaTrinketTypeID currentTrinketTypeID = currentItemDef.trinket.typeID;
								if (currentTrinketTypeID == itemDef.trinket.typeID)
								{
									DebugAssertIndex(exeData.equipment.spellcastingItemNames, currentTrinketTypeID);
									similarItemBaseName = exeData.equipment.spellcastingItemNames[currentTrinketTypeID];
								}
							}

							if (!similarItemBaseName.empty())
							{
								isSimilarItemAlreadyEquipped = true;
								break;
							}
						}
					}
				}

				if (isSimilarItemAlreadyEquipped)
				{
					std::string alreadyEquippedStr = String::replace(exeData.equipment.alreadyEquippedItem, "%s", similarItemBaseName.c_str());
					alreadyEquippedStr.pop_back(); // Clean up newline
					CharacterEquipmentUI::showItemDetail(alreadyEquippedStr.c_str(), CharacterEquipmentUiView::ItemDetailErrorTextColor);
					return;
				}

				auto unequipItemsIf = [listBoxElementInstID, &uiManager, &player, &playerInventory, &itemLibrary](const std::function<bool(const ItemDefinition&)> &predicate)
				{
					for (int i = 0; i < playerInventory.getTotalSlotCount(); i++)
					{
						ItemInstance &currentItemInst = playerInventory.getSlot(i);
						if (currentItemInst.isValid())
						{
							const ItemDefinition &currentItemDef = itemLibrary.getDefinition(currentItemInst.defID);
							if (predicate(currentItemDef))
							{
								currentItemInst.isEquipped = false;

								const int currentListBoxItemIndex = i; // @todo verify index mapping is correct once item dropping works
								const Color currentDisplayColor = InventoryUiView::getItemDisplayColor(currentItemInst, player);
								uiManager.setListBoxItemColorOverride(listBoxElementInstID, currentListBoxItemIndex, currentDisplayColor);
							}
						}
					}
				};

				const bool prevIsItemEquipped = itemInst.isEquipped;

				switch (itemType)
				{
				case ItemType::Accessory:
					break;
				case ItemType::Armor:
					unequipItemsIf([&itemDef](const ItemDefinition &def) { return (def.type == ItemType::Armor) && (def.armor.typeID == itemDef.armor.typeID); });
					break;
				case ItemType::Shield:
					unequipItemsIf([](const ItemDefinition &def)
					{
						if (def.type == ItemType::Shield)
						{
							return true;
						}
						else if (def.type == ItemType::Weapon)
						{
							return def.weapon.handCount == 2;
						}
						else
						{
							return false;
						}
					});
					break;
				case ItemType::Trinket:
					break;
				case ItemType::Weapon:
				{
					unequipItemsIf([&itemDef](const ItemDefinition &def)
					{
						if (def.type == ItemType::Shield)
						{
							return (itemDef.type == ItemType::Weapon) && (itemDef.weapon.handCount == 2);
						}
						else if (def.type == ItemType::Weapon)
						{
							return true;
						}
						else
						{
							return false;
						}
					});

					const ItemDefinitionID equippedItemDefID = player.getEquippedWeaponItemDefID();
					player.setWeaponAnimationFromItem(equippedItemDefID); // Resets to sheathed animation state.
					break;
				}
				default:
					DebugNotImplementedMsg(std::to_string(static_cast<int>(itemType)));
					break;
				}

				itemInst.isEquipped = !prevIsItemEquipped;

				const Color displayColor = InventoryUiView::getItemDisplayColor(itemInst, player);
				uiManager.setListBoxItemColorOverride(listBoxElementInstID, listBoxItemIndex, displayColor);
			}
			else if (mouseButtonType == MouseButtonType::Right)
			{
				DebugLogFormat("Not implemented: inspect item %d.", listBoxItemIndex);
			}
		};
	}
}

CharacterEquipmentUiState::CharacterEquipmentUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->itemDetailContextInstID = -1;
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

	UiContextInitInfo itemDetailContextInitInfo;
	itemDetailContextInitInfo.name = ContextName_ItemDetail;
	itemDetailContextInitInfo.drawOrder = 1;
	state.itemDetailContextInstID = uiManager.createContext(itemDetailContextInitInfo);
	uiManager.setContextEnabled(state.itemDetailContextInstID, false);

	CharacterSheetUiView::createOrUpdateEquipmentUiElements(CharacterEquipmentUI::ContextName, state.contextInstID, game);

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
	const std::vector<InventoryItemUiDefinition> itemUiDefs = InventoryUiModel::getPlayerInventoryItems(game);
	for (int i = 0; i < static_cast<int>(itemUiDefs.size()); i++)
	{
		const InventoryItemUiDefinition &itemUiDef = itemUiDefs[i];

		UiListBoxItem listBoxItem;
		listBoxItem.init(itemUiDef.text, itemUiDef.color, MakeInventoryListBoxItemCallback(game, inventoryListBoxElementInstID, i));

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

	if (state.itemDetailContextInstID >= 0)
	{
		uiManager.freeContext(state.itemDetailContextInstID, inputManager, renderer);
		state.itemDetailContextInstID = -1;
	}

	inputManager.setInputActionMapActive(InputActionMapName::CharacterEquipment, false);
}

void CharacterEquipmentUI::update(double dt)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const Player &player = game.player;
	const ItemInventory &playerInventory = player.inventory;
	bool isListBoxHighlightVisible = playerInventory.getOccupiedSlotCount() > 0;

	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_InventoryListBox);
	const Rect listBoxRect = uiManager.getTransformGlobalRect(listBoxElementInstID);
	const int listBoxHighlightedItemIndex = uiManager.getListBoxHighlightedItemIndex(listBoxElementInstID);	
	const Rect listBoxHighlightedItemGlobalRect = uiManager.getListBoxItemGlobalRect(listBoxElementInstID, listBoxHighlightedItemIndex);
	isListBoxHighlightVisible &= (listBoxHighlightedItemGlobalRect.getTop() >= listBoxRect.getTop()) && (listBoxHighlightedItemGlobalRect.getBottom() <= listBoxRect.getBottom());

	const UiElementInstanceID listBoxHighlightImageElementInstID = uiManager.getElementByName(ElementName_InventoryListBoxHighlight);
	uiManager.setElementActive(listBoxHighlightImageElementInstID, isListBoxHighlightVisible);

	const Int2 listBoxHighlightPosition = listBoxHighlightedItemGlobalRect.getTopLeft() - Int2(2, 2);
	uiManager.setTransformPosition(listBoxHighlightImageElementInstID, listBoxHighlightPosition);

	CharacterSheetUiView::createOrUpdateEquipmentUiElements(CharacterEquipmentUI::ContextName, state.contextInstID, game);
}

void CharacterEquipmentUI::showItemDetail(const char *text, Color textColor)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.itemDetailContextInstID, inputManager, renderer);

	UiElementInitInfo textBoxElementInitInfo;
	textBoxElementInitInfo.name = "CharacterEquipmentItemDetailTextBox";
	textBoxElementInitInfo.position = CharacterEquipmentUiView::ItemDetailCenterPoint;
	textBoxElementInitInfo.pivotType = CharacterEquipmentUiView::ItemDetailPivotType;
	textBoxElementInitInfo.drawOrder = 0;

	UiTextBoxInitInfo textBoxInitInfo;
	textBoxInitInfo.text = text;
	textBoxInitInfo.fontName = CharacterEquipmentUiView::ItemDetailFontName;
	textBoxInitInfo.defaultColor = textColor;
	textBoxInitInfo.alignment = CharacterEquipmentUiView::ItemDetailTextAlignment;
	textBoxInitInfo.lineSpacing = CharacterEquipmentUiView::ItemDetailLineSpacing;
	const UiElementInstanceID textBoxElementInstID = uiManager.createTextBox(textBoxElementInitInfo, textBoxInitInfo, state.itemDetailContextInstID, renderer);
	const Rect textBoxRect = uiManager.getTransformGlobalRect(textBoxElementInstID);

	UiElementInitInfo backButtonElementInitInfo;
	backButtonElementInitInfo.name = "CharacterEquipmentItemDetailBackButton";
	backButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	backButtonElementInitInfo.drawOrder = 1;

	auto backButtonCallback = [](MouseButtonType)
	{
		CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();
	};

	UiButtonInitInfo backButtonInitInfo;
	backButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	backButtonInitInfo.callback = backButtonCallback;
	uiManager.createButton(backButtonElementInitInfo, backButtonInitInfo, state.itemDetailContextInstID);

	auto inputActionCallback = [backButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			backButtonCallback(MouseButtonType::Left);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, inputActionCallback, ContextName_ItemDetail, inputManager);
	uiManager.setContextEnabled(state.itemDetailContextInstID, true);
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
