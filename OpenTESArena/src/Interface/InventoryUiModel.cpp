#include "InventoryUiModel.h"
#include "InventoryUiView.h"
#include "../Items/ItemLibrary.h"

void InventoryUiModel::ItemUiDefinition::init(const std::string &text, const Color &color)
{
	this->text = text;
	this->color = color;
}

Buffer<InventoryUiModel::ItemUiDefinition> InventoryUiModel::getPlayerInventoryItems(Game &game)
{
	// @todo: actually grab from player inventory
	std::vector<const ItemDefinition*> itemDefs;
	const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
	for (int i = 0; i < itemLibrary.getCount(); i++)
	{
		const ItemDefinition &itemDef = itemLibrary.getDefinition(i);
		itemDefs.emplace_back(&itemDef);
	}

	auto getItemDisplayName = [](const ItemDefinition &itemDef) -> std::string
	{
		switch (itemDef.type)
		{
		case ItemType::Accessory:
			return itemDef.accessory.name;
		case ItemType::Armor:
			return itemDef.armor.name;
		case ItemType::Consumable:
			return itemDef.consumable.name;
		case ItemType::Misc:
			return itemDef.misc.name;
		case ItemType::Shield:
			return itemDef.shield.name;
		case ItemType::Trinket:
			return itemDef.trinket.name;
		case ItemType::Weapon:
			return itemDef.weapon.name;
		default:
			DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(itemDef.type)));
		}
	};

	const int elementCount = static_cast<int>(itemDefs.size());
	Buffer<ItemUiDefinition> buffer(elementCount);
	for (int i = 0; i < elementCount; i++)
	{
		const ItemDefinition &itemDef = *itemDefs[i];
		const std::string itemDisplayName = getItemDisplayName(itemDef);
		const Color &itemTextColor = InventoryUiView::PlayerInventoryEquipmentColor;

		ItemUiDefinition itemUiDef;
		itemUiDef.init(itemDisplayName, itemTextColor);

		buffer.set(i, std::move(itemUiDef));
	}

	return buffer;
}
