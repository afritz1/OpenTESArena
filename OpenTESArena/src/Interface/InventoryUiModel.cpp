#include <array>

#include "InventoryUiModel.h"
#include "InventoryUiView.h"

void InventoryUiModel::ItemUiDefinition::init(std::string &&text, const Color &color)
{
	this->text = std::move(text);
	this->color = color;
}

Buffer<InventoryUiModel::ItemUiDefinition> InventoryUiModel::getPlayerInventoryItems(Game &game)
{
	const std::array<std::pair<std::string, Color>, 10> elements =
	{
		{
			{ "Test slot 1", InventoryUiView::PlayerInventoryEquipmentColor },
			{ "Test slot 2", InventoryUiView::PlayerInventoryEquipmentEquippedColor },
			{ "Test slot 3", InventoryUiView::PlayerInventoryMagicItemColor },
			{ "Test slot 4", InventoryUiView::PlayerInventoryMagicItemEquippedColor },
			{ "Test slot 5", InventoryUiView::PlayerInventoryUnequipableColor },
			{ "Test slot 6", InventoryUiView::PlayerInventoryUnequipableColor },
			{ "Test slot 7", InventoryUiView::PlayerInventoryEquipmentColor },
			{ "Test slot 8", InventoryUiView::PlayerInventoryEquipmentColor },
			{ "Test slot 9", InventoryUiView::PlayerInventoryMagicItemColor },
			{ "Test slot 10", InventoryUiView::PlayerInventoryMagicItemEquippedColor }
		}
	};

	const int elementCount = static_cast<int>(elements.size());
	Buffer<ItemUiDefinition> buffer(elementCount);
	for (int i = 0; i < elementCount; i++)
	{
		const auto &pair = elements[i];

		ItemUiDefinition itemUiDef;
		itemUiDef.init(std::string(pair.first), pair.second);

		buffer.set(i, std::move(itemUiDef));
	}

	return buffer;
}
