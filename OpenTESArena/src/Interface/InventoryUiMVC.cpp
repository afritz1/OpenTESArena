#include <cstdio>

#include "InventoryUiMVC.h"
#include "../Game/Game.h"
#include "../Items/ItemLibrary.h"
#include "../Items/ItemInstance.h"
#include "../Player/Player.h"
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
		const Color &itemTextColor = InventoryUiView::getItemDisplayColor(itemInst);

		ItemUiDefinition itemUiDef;
		itemUiDef.init(itemDisplayName, itemTextColor);

		buffer.set(i, std::move(itemUiDef));
	}

	return buffer;
}

const Color &InventoryUiView::getItemDisplayColor(const ItemInstance &itemInst)
{
	// @todo magic items

	// @todo: class non-equippable items

	if (itemInst.isEquipped)
	{
		return InventoryUiView::PlayerInventoryEquipmentEquippedColor;
	}

	return InventoryUiView::PlayerInventoryEquipmentColor;
}
