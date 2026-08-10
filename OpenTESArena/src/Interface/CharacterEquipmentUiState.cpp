#include "CharacterEquipmentUiState.h"
#include "CharacterSheetUiMVC.h"
#include "CharacterUiState.h"
#include "CommonUiMVC.h"
#include "InventoryUiMVC.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Items/ItemLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontLibrary.h"
#include "../World/MapType.h"

#include "components/utilities/String.h"

namespace
{
	constexpr char ContextName_ItemDetail[] = "CharacterEquipmentItemDetail";
	constexpr char ContextName_DropItemModal[] = "CharacterEquipmentDropItemModal";

	constexpr char ElementName_InventoryListBox[] = "CharacterEquipmentInventoryListBox";	
	constexpr char ElementName_InventoryListBoxHighlight[] = "CharacterEquipmentInventoryListBoxHighlight";

	UiListBoxItemCallback MakeInventoryListBoxItemCallback(Game &game, UiElementInstanceID listBoxElementInstID, int inventorySlotIndex)
	{
		return [&game, listBoxElementInstID, inventorySlotIndex](MouseButtonType mouseButtonType)
		{
			UiManager &uiManager = game.uiManager;
			const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

			Player &player = game.player;
			ItemInventory &playerInventory = player.inventory;
			const CharacterClassDefinition &charClassDef = CharacterClassLibrary::getInstance().getDefinition(player.charClassDefID);

			const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
			ItemInstance &itemInst = playerInventory.getSlot(inventorySlotIndex);
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
					if (i == inventorySlotIndex)
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
					for (int currentSlotIndex = 0; currentSlotIndex < playerInventory.getTotalSlotCount(); currentSlotIndex++)
					{
						ItemInstance &currentItemInst = playerInventory.getSlot(currentSlotIndex);
						if (currentItemInst.isValid())
						{
							const ItemDefinition &currentItemDef = itemLibrary.getDefinition(currentItemInst.defID);
							if (predicate(currentItemDef))
							{
								currentItemInst.isEquipped = false;

								const int currentListBoxItemIndex = playerInventory.getValidSlotCountBeforeIndex(currentSlotIndex);
								const Color currentDisplayColor = InventoryUiView::getItemDisplayColor(currentSlotIndex, player);
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

				const int listBoxItemIndex = playerInventory.getValidSlotCountBeforeIndex(inventorySlotIndex);
				const Color displayColor = InventoryUiView::getItemDisplayColor(inventorySlotIndex, player);
				uiManager.setListBoxItemColorOverride(listBoxElementInstID, listBoxItemIndex, displayColor);
			}
			else if (mouseButtonType == MouseButtonType::Right)
			{
				const std::string itemDisplayName = itemDef.getDisplayNameWithoutQty();
				const int itemConditionNameIndex = 7; // @todo condition value logic

				std::string itemDetailText;
				switch (itemType)
				{
				case ItemType::Accessory:
				{
					const AccessoryItemDefinition &accessoryItemDef = itemDef.accessory;
					if (accessoryItemDef.isAttributeEnhancing())
					{
						const int accessoryStatBonusAmount = 0; // @todo stat bonuses
						const Span<const std::string> attributeNames = exeData.entities.attributeNames;
						const std::string accessoryStatName = attributeNames[accessoryItemDef.attributeID];
						const std::string accessoryStatBonusString = String::format(exeData.equipment.itemDetailStatBonus.c_str(), accessoryStatBonusAmount, accessoryStatName.c_str());
						itemDetailText = String::format("%s\n%s", itemDisplayName.c_str(), accessoryStatBonusString.c_str());
					}
					else
					{
						const int accessoryArmorValue = accessoryItemDef.armorClass;
						const std::string accessoryArmorValueString = String::format(exeData.equipment.itemDetailArmorNoWeight.c_str(), accessoryArmorValue);
						itemDetailText = String::format("%s\n%s", itemDisplayName.c_str(), accessoryArmorValueString.c_str());
					}
					
					break;
				}
				case ItemType::Armor:
				{
					const ArmorItemDefinition &armorItemDef = itemDef.armor;
					const double armorWeightKgs = armorItemDef.weight;
					const int armorRating = armorItemDef.armorClass;
					
					std::string armorRatingAndWeightString;					
					if (armorWeightKgs > 0.0)
					{
						const std::string armorRatingString = String::format(exeData.equipment.itemDetailArmor.c_str(), armorRating);
						const std::string armorWeightString = String::format("%.1f kgs", armorWeightKgs);
						armorRatingAndWeightString = String::format("%s%s", armorRatingString.c_str(), armorWeightString.c_str());
					}
					else
					{
						armorRatingAndWeightString = String::format(exeData.equipment.itemDetailArmorNoWeight.c_str(), armorRating);
					}

					const std::string armorConditionName = exeData.equipment.itemConditionNames[itemConditionNameIndex];
					const std::string armorConditionString = String::format(exeData.equipment.itemDetailCondition.c_str(), armorConditionName.c_str());
					itemDetailText = String::format("%s\n%s\n%s", itemDisplayName.c_str(), armorRatingAndWeightString.c_str(), armorConditionString.c_str());
					break;
				}
				case ItemType::Consumable:
				{
					std::string potionDetailString = String::replace(exeData.equipment.itemDetailPotion, '\r', '\n');
					potionDetailString.pop_back(); // Clean up newline.
					itemDetailText = String::format("%s\n%s", itemDisplayName.c_str(), potionDetailString.c_str());
					break;
				}
				case ItemType::Misc:
					itemDetailText = String::format("%s", itemDisplayName.c_str());
					break;
				case ItemType::Shield:
				{
					const ShieldItemDefinition &shieldItemDef = itemDef.shield;
					const double shieldWeightKgs = shieldItemDef.weight;
					const int shieldArmorRating = shieldItemDef.armorClass;
					const std::string shieldArmorRatingString = String::format(exeData.equipment.itemDetailArmor.c_str(), shieldArmorRating);
					const std::string shieldWeightString = String::format("%.1f kgs", shieldWeightKgs);
					const std::string shieldConditionName = exeData.equipment.itemConditionNames[itemConditionNameIndex];
					const std::string shieldConditionString = String::format(exeData.equipment.itemDetailCondition.c_str(), shieldConditionName.c_str());
					itemDetailText = String::format("%s\n%s%s\n%s", itemDisplayName.c_str(), shieldArmorRatingString.c_str(), shieldWeightString.c_str(), shieldConditionString.c_str());
					break;
				}
				case ItemType::Trinket:
				{
					const int trinketChargesLeft = 0; // @todo charges
					const std::string chargesLeftString = String::format(exeData.equipment.itemDetailChargesLeft.c_str(), trinketChargesLeft);
					itemDetailText = String::format("%s\n%s", itemDisplayName.c_str(), chargesLeftString.c_str());
					break;
				}
				case ItemType::Weapon:
				{
					const WeaponItemDefinition &weaponItemDef = itemDef.weapon;
					const double weaponWeightKgs = weaponItemDef.weight;
					const std::string weaponDamageString = String::format(exeData.equipment.itemDetailWeapon.c_str(), weaponItemDef.damageMin, weaponItemDef.damageMax);
					const std::string weaponWeightString = String::format("%.1f kgs", weaponWeightKgs);
					const std::string weaponConditionName = exeData.equipment.itemConditionNames[itemConditionNameIndex];
					const std::string weaponConditionString = String::format(exeData.equipment.itemDetailCondition.c_str(), weaponConditionName.c_str());
					itemDetailText = String::format("%s\n%s%s\n%s", itemDisplayName.c_str(), weaponDamageString.c_str(), weaponWeightString.c_str(), weaponConditionString.c_str());
					break;
				}
				default:
					DebugNotImplementedMsg(std::to_string(static_cast<int>(itemType)));
					break;
				}

				CharacterEquipmentUI::showItemDetail(itemDetailText.c_str(), CharacterEquipmentUiView::ItemDetailDefaultTextColor);
			}
		};
	}
}

CharacterEquipmentUiState::CharacterEquipmentUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->itemDetailContextInstID = -1;
	this->dropItemModalContextInstID = -1;
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

	UiContextInitInfo dropItemModalContextInitInfo;
	dropItemModalContextInitInfo.name = ContextName_DropItemModal;
	dropItemModalContextInitInfo.drawOrder = 2;
	state.dropItemModalContextInstID = uiManager.createContext(dropItemModalContextInitInfo);
	uiManager.setContextEnabled(state.dropItemModalContextInstID, false);

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

	const UiElementInstanceID inventoryListBoxElementInstID = uiManager.getElementByName(ElementName_InventoryListBox);

	const Player &player = game.player;
	const ItemInventory &playerInventory = player.inventory;
	for (int i = 0; i < playerInventory.getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = playerInventory.getSlot(i);
		if (!itemInst.isValid())
		{
			continue;
		}

		const std::string itemText = InventoryUiModel::getItemText(i, playerInventory);
		const Color itemColor = InventoryUiView::getItemDisplayColor(i, player);

		UiListBoxItem listBoxItem;
		listBoxItem.init(itemText, itemColor, MakeInventoryListBoxItemCallback(game, inventoryListBoxElementInstID, i));

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

	if (state.dropItemModalContextInstID >= 0)
	{
		uiManager.freeContext(state.dropItemModalContextInstID, inputManager, renderer);
		state.dropItemModalContextInstID = -1;
	}

	Player &player = game.player;
	player.inventory.compact(); // Avoid keeping invalid slots around so newly-inserted items go at the back.

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
	backButtonInitInfo.mouseButtonFlags = CommonUiView::PopUpMouseButtonTypeFlags;
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

void CharacterEquipmentUI::showDropItemMessageBox(const char *titleText, const std::function<void()> &confirmCallback)
{
	CharacterEquipmentUiState &state = CharacterEquipmentUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.dropItemModalContextInstID, inputManager, renderer);

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	
	const char *messageBoxButtonTexts[] =
	{
		// @todo get from ExeData
		"Yes",
		"No"
	};

	const UiButtonCallback messageBoxYesButtonCallback = [&uiManager, confirmCallback](MouseButtonType)
	{
		uiManager.disableTopMostContext();
		confirmCallback();
	};

	const UiButtonCallback messageBoxNoButtonCallback = [&uiManager](MouseButtonType)
	{
		uiManager.disableTopMostContext();
	};

	const UiButtonCallback messageBoxButtonCallbacks[] = { messageBoxYesButtonCallback, messageBoxNoButtonCallback };
	constexpr int messageBoxButtonCount = static_cast<int>(std::size(messageBoxButtonTexts));

	const int titleImageTextureHeight = 30;
	const int totalMessageBoxHeight = titleImageTextureHeight * (messageBoxButtonCount + 1);
	const int titleTextBoxPositionY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (totalMessageBoxHeight / 2) + (titleImageTextureHeight / 2);

	UiElementInitInfo titleTextBoxElementInitInfo;
	titleTextBoxElementInitInfo.name = "CharacterEquipmentDropItemModalTitleTextBox";
	titleTextBoxElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, titleTextBoxPositionY);
	titleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	titleTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo titleTextBoxInitInfo;
	titleTextBoxInitInfo.text = titleText;
	titleTextBoxInitInfo.fontName = ArenaFontName::Arena;
	titleTextBoxInitInfo.defaultColor = CharacterEquipmentUiView::ItemDetailDefaultTextColor;
	titleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.createTextBox(titleTextBoxElementInitInfo, titleTextBoxInitInfo, state.dropItemModalContextInstID, renderer);
	const Rect titleTextBoxGlobalRect = uiManager.getTransformGlobalRect(titleTextBoxElementInstID);

	int widestButtonTextBoxWidth = titleTextBoxGlobalRect.width;
	for (int i = 0; i < messageBoxButtonCount; i++)
	{
		UiElementInitInfo messageBoxButtonTextBoxElementInitInfo;
		messageBoxButtonTextBoxElementInitInfo.name = String::format("CharacterEquipmentDropItemModalButton%dTextBox", i);
		messageBoxButtonTextBoxElementInitInfo.position = titleTextBoxElementInitInfo.position + Int2(0, titleImageTextureHeight * (i + 1));
		messageBoxButtonTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
		messageBoxButtonTextBoxElementInitInfo.drawOrder = 1;

		UiTextBoxInitInfo messageBoxButtonTextBoxInitInfo;
		messageBoxButtonTextBoxInitInfo.text = messageBoxButtonTexts[i];
		messageBoxButtonTextBoxInitInfo.fontName = titleTextBoxInitInfo.fontName;
		messageBoxButtonTextBoxInitInfo.defaultColor = titleTextBoxInitInfo.defaultColor;
		messageBoxButtonTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
		const UiElementInstanceID messageBoxButtonTextBoxElementInstID = uiManager.createTextBox(messageBoxButtonTextBoxElementInitInfo, messageBoxButtonTextBoxInitInfo, state.dropItemModalContextInstID, renderer);
		const Rect messageBoxButtonTextBoxGlobalRect = uiManager.getTransformGlobalRect(messageBoxButtonTextBoxElementInstID);
		widestButtonTextBoxWidth = std::max(widestButtonTextBoxWidth, messageBoxButtonTextBoxGlobalRect.width);
	}

	UiElementInitInfo titleImageElementInitInfo;
	titleImageElementInitInfo.name = "CharacterEquipmentDropItemModalTitleImage";
	titleImageElementInitInfo.position = titleTextBoxElementInitInfo.position;
	titleImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
	titleImageElementInitInfo.size = Int2(widestButtonTextBoxWidth + 14, titleImageTextureHeight);
	titleImageElementInitInfo.pivotType = UiPivotType::Middle;

	const UiTextureID titleImageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::Dark, titleImageElementInitInfo.size.x, titleImageElementInitInfo.size.y, textureManager, renderer);
	uiManager.createImage(titleImageElementInitInfo, titleImageTextureID, state.dropItemModalContextInstID, renderer);

	for (int i = 0; i < messageBoxButtonCount; i++)
	{
		UiElementInitInfo messageBoxButtonImageElementInitInfo;
		messageBoxButtonImageElementInitInfo.name = "CharacterEquipmentDropItemModalButton" + std::to_string(i) + "Image";
		messageBoxButtonImageElementInitInfo.position = titleTextBoxElementInitInfo.position + Int2(0, titleImageTextureHeight * (i + 1));
		messageBoxButtonImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
		messageBoxButtonImageElementInitInfo.size = titleImageElementInitInfo.size;
		messageBoxButtonImageElementInitInfo.pivotType = UiPivotType::Middle;
		uiManager.createImage(messageBoxButtonImageElementInitInfo, titleImageTextureID, state.dropItemModalContextInstID, renderer);

		UiElementInitInfo messageBoxButtonElementInitInfo;
		messageBoxButtonElementInitInfo.name = "CharacterEquipmentDropItemModalButton" + std::to_string(i);
		messageBoxButtonElementInitInfo.position = messageBoxButtonImageElementInitInfo.position;
		messageBoxButtonElementInitInfo.pivotType = UiPivotType::Middle;

		UiButtonInitInfo messageBoxButtonInitInfo;
		messageBoxButtonInitInfo.callback = messageBoxButtonCallbacks[i];
		messageBoxButtonInitInfo.contentElementName = messageBoxButtonImageElementInitInfo.name;
		uiManager.createButton(messageBoxButtonElementInitInfo, messageBoxButtonInitInfo, state.dropItemModalContextInstID);
	}

	UiElementInitInfo backButtonElementInitInfo;
	backButtonElementInitInfo.name = "CharacterEquipmentDropItemModalBackButton";
	backButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	UiButtonInitInfo backButtonInitInfo;
	backButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	backButtonInitInfo.callback = messageBoxNoButtonCallback;
	uiManager.createButton(backButtonElementInitInfo, backButtonInitInfo, state.dropItemModalContextInstID);

	auto backInputActionCallback = [messageBoxNoButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			messageBoxNoButtonCallback(MouseButtonType::Right);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, backInputActionCallback, ContextName_DropItemModal, inputManager);

	uiManager.setContextEnabled(state.dropItemModalContextInstID, true);
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
	UiManager &uiManager = game.uiManager;

	Player &player = game.player;
	ItemInventory &playerInventory = player.inventory;
	if (playerInventory.getOccupiedSlotCount() == 0)
	{
		return;
	}

	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_InventoryListBox);
	const int listBoxHighlightedItemIndex = uiManager.getListBoxHighlightedItemIndex(listBoxElementInstID);

	int highlightedSlotIndex = 0;
	int validSlotCount = 0;
	for (int i = 0; i < playerInventory.getTotalSlotCount(); i++)
	{
		if (playerInventory.getSlot(i).isValid())
		{
			if (validSlotCount == listBoxHighlightedItemIndex)
			{
				highlightedSlotIndex = i;
				break;
			}

			validSlotCount++;
		}
	}

	ItemInstance &highlightedItemInst = playerInventory.getSlot(highlightedSlotIndex);
	DebugAssert(highlightedItemInst.isValid());
	const bool isHighlightedItemEquipped = highlightedItemInst.isEquipped;
	const ItemDefinitionID highlightedItemDefID = highlightedItemInst.defID;
	const ItemDefinition &highlightedItemDef = ItemLibrary::getInstance().getDefinition(highlightedItemDefID);
	const ItemType highlightedItemType = highlightedItemDef.type;

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	
	const bool canItemTypeBeDropped = highlightedItemType != ItemType::Misc;
	if (!canItemTypeBeDropped)
	{
		CharacterEquipmentUI::showItemDetail(exeData.equipment.dropItemNotDroppable.c_str(), CharacterEquipmentUiView::ItemDetailErrorTextColor);
		return;
	}

	const std::string highlightedItemDisplayName = highlightedItemDef.getDisplayNameWithoutQty();
	const bool isUnequippingWeapon = isHighlightedItemEquipped && (highlightedItemType == ItemType::Weapon);

	auto removeItemFromInventoryCallback = [&uiManager, &player, listBoxElementInstID, listBoxHighlightedItemIndex, &highlightedItemInst, &highlightedItemDef, isUnequippingWeapon]()
	{
		if (highlightedItemInst.stackAmount > 1)
		{
			highlightedItemInst.stackAmount--;

			const std::string newHighlightedItemDisplayName = highlightedItemDef.getDisplayNameWithQty(highlightedItemInst.stackAmount);
			uiManager.setListBoxItemText(listBoxElementInstID, listBoxHighlightedItemIndex, newHighlightedItemDisplayName.c_str());
		}
		else
		{
			highlightedItemInst.clear();
			uiManager.eraseListBoxItem(listBoxElementInstID, listBoxHighlightedItemIndex);

			if (isUnequippingWeapon)
			{
				player.setWeaponAnimationFromItem(ArenaItemUtils::FistsWeaponID);
			}
		}
	};

	const GameState &gameState = game.gameState;
	const MapType mapType = gameState.getActiveMapType();
	const bool isPlacingItemInContainerAllowed = mapType == MapType::Interior;
	if (!isPlacingItemInContainerAllowed)
	{
		std::string text = String::replace(exeData.equipment.dropItemPermanent, '\r', '\n');
		text.pop_back(); // Clean up newline
		CharacterEquipmentUI::showDropItemMessageBox(text.c_str(), removeItemFromInventoryCallback);
		return;
	}

	const double ceilingScale = gameState.getActiveCeilingScale();
	const WorldInt3 playerWorldVoxel = VoxelUtils::pointToVoxel(player.getEyePosition(), ceilingScale);
	const WorldInt2 playerWorldVoxelXZ = playerWorldVoxel.getXZ();
	const CoordInt3 playerVoxelCoord = VoxelUtils::worldVoxelToCoord(playerWorldVoxel);
	const VoxelInt3 playerVoxel = playerVoxelCoord.voxel;
	const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
	const VoxelChunk &playerVoxelChunk = voxelChunkManager.getChunkAtPosition(playerVoxelCoord.chunk);
	
	bool isPlayerVoxelValidForDroppingItem = true;
	
	const bool isPlayerVoxelValid = playerVoxelChunk.isValidVoxel(playerVoxel.x, playerVoxel.y, playerVoxel.z);
	isPlayerVoxelValidForDroppingItem &= isPlayerVoxelValid && (playerVoxel.y == 1);

	if (isPlayerVoxelValidForDroppingItem)
	{
		const VoxelShapeDefID playerVoxelShapeDefID = playerVoxelChunk.shapeDefIDs.get(playerVoxel.x, playerVoxel.y, playerVoxel.z);
		const VoxelShapeDefinition &playerVoxelShapeDef = playerVoxelChunk.shapeDefs[playerVoxelShapeDefID];
		isPlayerVoxelValidForDroppingItem &= playerVoxelShapeDef.mesh.isEmpty();

		const VoxelTraitsDefID floorVoxelTraitsDefID = playerVoxelChunk.traitsDefIDs.get(playerVoxel.x, 0, playerVoxel.z);
		const VoxelTraitsDefinition &floorVoxelTraitsDef = playerVoxelChunk.traitsDefs[floorVoxelTraitsDefID];
		isPlayerVoxelValidForDroppingItem &= floorVoxelTraitsDef.type == ArenaVoxelType::Floor;
	}
	
	EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	EntityInstanceID occupyingEntityInstID = -1;

	if (isPlayerVoxelValidForDroppingItem)
	{
		bool isOccupyingEntityValidContainer = false;

		std::unordered_map<WorldInt2, EntityInstanceID> &occupiedVoxels = entityChunkManager.occupiedVoxels;
		const auto occupiedVoxelIter = occupiedVoxels.find(playerWorldVoxelXZ);
		if (occupiedVoxelIter != occupiedVoxels.end())
		{
			occupyingEntityInstID = occupiedVoxelIter->second;

			const EntityInstance &occupyingEntityInst = entityChunkManager.entities.get(occupyingEntityInstID);
			bool isValidContainer = occupyingEntityInst.isTransformStatic() && occupyingEntityInst.hasInventory();
			if (occupyingEntityInst.canBeLocked())
			{
				const EntityLockState &lockState = entityChunkManager.lockStates.get(occupyingEntityInst.lockStateID);
				isValidContainer &= !lockState.isLocked;
			}

			isOccupyingEntityValidContainer = isValidContainer;
		}

		if (occupyingEntityInstID < 0)
		{
			EntityDefinitionKey containerEntityDefKey;
			containerEntityDefKey.initContainer(true);

			EntityDefID containerEntityDefID;
			if (!EntityDefinitionLibrary::getInstance().tryGetDefinitionID(containerEntityDefKey, &containerEntityDefID))
			{
				DebugLogError("Couldn't find entity def ID for player item drop container.");
				return;
			}

			RenderEntityManager &renderEntityManager = game.sceneManager.renderEntityManager;
			renderEntityManager.loadMaterialsForEntity(containerEntityDefID, game.textureManager, game.renderer);

			const EntityDefinition &containerEntityDef = entityChunkManager.getEntityDef(containerEntityDefID);
			const EntityAnimationDefinition &containerAnimDef = containerEntityDef.animDef;
			const WorldDouble2 containerPositionXZ = VoxelUtils::getVoxelCenter(playerWorldVoxelXZ);

			EntityInitInfo containerEntityInitInfo;
			containerEntityInitInfo.defID = containerEntityDefID;
			containerEntityInitInfo.feetPosition = WorldDouble3(containerPositionXZ.x, ceilingScale, containerPositionXZ.y);
			containerEntityInitInfo.initialAnimStateIndex = *containerAnimDef.findStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
			containerEntityInitInfo.isSensorCollider = true;
			containerEntityInitInfo.hasInventory = true;
			occupyingEntityInstID = entityChunkManager.createEntity(containerEntityInitInfo, game.random, game.physicsSystem, game.renderer);

			// Have to manually update occupied voxels while in the UI in case player drops multiple items at once, otherwise we get N piles.
			DebugAssert(occupiedVoxelIter == occupiedVoxels.end());
			occupiedVoxels.emplace(playerWorldVoxelXZ, occupyingEntityInstID);

			isOccupyingEntityValidContainer = true;
		}

		isPlayerVoxelValidForDroppingItem &= isOccupyingEntityValidContainer;
	}
	
	if (!isPlayerVoxelValidForDroppingItem)
	{
		CharacterEquipmentUI::showItemDetail(exeData.equipment.dropItemNoRoom.c_str(), CharacterEquipmentUiView::ItemDetailErrorTextColor);
		return;
	}

	auto dropItemIntoContainerCallback = [highlightedItemDefID, &entityChunkManager, occupyingEntityInstID, removeItemFromInventoryCallback]()
	{
		const EntityInstance &containerEntityInst = entityChunkManager.entities.get(occupyingEntityInstID);
		ItemInventory &containerInventory = entityChunkManager.itemInventories.get(containerEntityInst.itemInventoryInstID);
		containerInventory.insert(highlightedItemDefID);
		removeItemFromInventoryCallback();
	};

	std::string text = String::replace(exeData.equipment.dropItem, "%s", highlightedItemDisplayName.c_str());
	text = String::replace(text, '\r', '\n');
	text.pop_back(); // Clean up newline
	CharacterEquipmentUI::showDropItemMessageBox(text.c_str(), dropItemIntoContainerCallback);
}

void CharacterEquipmentUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		CharacterEquipmentUI::onExitButtonSelected(MouseButtonType::Left);
	}
}
