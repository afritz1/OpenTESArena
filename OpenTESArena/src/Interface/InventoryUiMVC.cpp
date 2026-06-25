#include <cstdio>

#include "InventoryUiMVC.h"
#include "../Game/Game.h"
#include "../Items/ItemLibrary.h"
#include "../Items/ItemInstance.h"
#include "../Player/Player.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextRenderUtils.h"

void InventoryUiModel::ItemUiDefinition::init(const std::string &text, const Color &color)
{
	this->text = text;
	this->color = color;
}

Buffer<InventoryUiModel::ItemUiDefinition> InventoryUiModel::getPlayerInventoryItems(Game &game)
{
	const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
	const Player &player = game.player;
	const ItemInventory &playerInventory = player.inventory;
	const int itemCount = playerInventory.getTotalSlotCount();
	Buffer<ItemUiDefinition> buffer(itemCount);
	for (int i = 0; i < itemCount; i++)
	{
		const ItemInstance &itemInst = playerInventory.getSlot(i);
		if (!itemInst.isValid())
		{
			continue;
		}

		const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);

		char itemDisplayName[64];
		std::snprintf(std::begin(itemDisplayName), std::size(itemDisplayName), "%s (%.1fkg)", itemDef.getDisplayName(itemInst.stackAmount).c_str(), itemDef.getWeight());
		const Color &itemTextColor = InventoryUiView::getItemDisplayColor(itemInst, player);

		ItemUiDefinition itemUiDef;
		itemUiDef.init(itemDisplayName, itemTextColor);

		buffer.set(i, std::move(itemUiDef));
	}

	return buffer;
}

Color InventoryUiView::getItemDisplayColor(const ItemInstance &itemInst, const Player &player)
{
	constexpr Color unequippedColor = InventoryUiView::ItemDefaultColor;
	constexpr Color equippedColor = InventoryUiView::ItemEquippedColor;
	constexpr Color unequippedMagicColor = InventoryUiView::ItemMagicColor;
	constexpr Color equippedMagicColor = InventoryUiView::ItemMagicEquippedColor;
	constexpr Color unequippableColor = InventoryUiView::ItemUnequippableColor;

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemInst.defID);
	const CharacterClassDefinition &charClassDef = CharacterClassLibrary::getInstance().getDefinition(player.charClassDefID);

	const bool isItemEquipped = itemInst.isEquipped;

	switch (itemDef.type)
	{
	case ItemType::Accessory:
		// @todo magic color if unidentified
		return isItemEquipped ? equippedColor : unequippedColor;
	case ItemType::Armor:
	{
		// @todo magic color if unidentified

		const ArmorItemDefinition &armorDef = itemDef.armor;
		if (!charClassDef.isArmorMaterialAllowed(armorDef.materialType))
		{
			return unequippableColor;
		}

		// @todo magic items
		return isItemEquipped ? equippedColor : unequippedColor;
	}
	case ItemType::Consumable:
		// @todo magic color if unidentified
		return unequippableColor;
	case ItemType::Gold:
		return unequippableColor; // Only for display in loot UI.
	case ItemType::Misc:
		return unequippableColor;
	case ItemType::Shield:
	{
		// @todo magic color if unidentified
		if (!charClassDef.isShieldTypeAllowed(itemDef.shield.armorTypeID))
		{
			return unequippableColor;
		}

		return isItemEquipped ? equippedColor : unequippedColor;
	}
	case ItemType::Trinket:
		return isItemEquipped ? equippedMagicColor : unequippedMagicColor;
	case ItemType::Weapon:
	{
		// @todo magic color if unidentified
		if (!charClassDef.isWeaponTypeAllowed(itemDef.weapon.typeID))
		{
			return unequippableColor;
		}

		// @todo magic items
		return isItemEquipped ? equippedColor : unequippedColor;
	}
	default:
		DebugUnhandledReturnMsg(Color, std::to_string(static_cast<int>(itemDef.type)));
	}
}
