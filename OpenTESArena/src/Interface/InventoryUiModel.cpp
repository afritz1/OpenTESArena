#include <cstdio>

#include "InventoryUiModel.h"
#include "InventoryUiView.h"
#include "../Entities/Player.h"
#include "../Game/Game.h"
#include "../Items/ItemLibrary.h"

void InventoryUiModel::ItemUiDefinition::init(const std::string &text, const Color &color)
{
	this->text = text;
	this->color = color;
}

Buffer<InventoryUiModel::ItemUiDefinition> InventoryUiModel::getPlayerInventoryItems(Game &game)
{
	const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
	const Player &player = game.player;

	std::vector<const ItemDefinition*> itemDefs;
	for (int i = 0; i < player.inventory.getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = player.inventory.getSlot(i);
		if (itemInst.isValid())
		{
			const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);
			itemDefs.emplace_back(&itemDef);
		}
	}

	const int elementCount = static_cast<int>(itemDefs.size());
	Buffer<ItemUiDefinition> buffer(elementCount);
	for (int i = 0; i < elementCount; i++)
	{
		const ItemDefinition &itemDef = *itemDefs[i];

		char itemDisplayName[64];
		std::snprintf(std::begin(itemDisplayName), std::size(itemDisplayName), "%s (%.1fkg)", itemDef.getDisplayName().c_str(), itemDef.getWeight());
		const Color &itemTextColor = InventoryUiView::PlayerInventoryEquipmentColor;

		ItemUiDefinition itemUiDef;
		itemUiDef.init(itemDisplayName, itemTextColor);

		buffer.set(i, std::move(itemUiDef));
	}

	return buffer;
}
