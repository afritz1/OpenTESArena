#include <cstdio>

#include "InventoryUiModel.h"
#include "InventoryUiView.h"
#include "../Game/Game.h"
#include "../Items/ItemLibrary.h"
#include "../Player/Player.h"

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
