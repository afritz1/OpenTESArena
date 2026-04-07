#pragma once

#include <string>

#include "../Math/Rect.h"
#include "../Utilities/Color.h"

#include "components/utilities/Buffer.h"

class Game;

struct ItemInstance;

namespace InventoryUiModel
{
	// @todo: not sure if this is final -- I think I want the text (data) and color (presentation) to be separate.
	// - maybe it could be more like "getItemText(int)" and "getItemColor(int)".
	struct ItemUiDefinition
	{
		std::string text;
		Color color;

		void init(const std::string &text, const Color &color);
	};

	Buffer<ItemUiDefinition> getPlayerInventoryItems(Game &game);
}

namespace InventoryUiView
{
	const Color PlayerInventoryEquipmentColor(211, 142, 0);
	const Color PlayerInventoryEquipmentEquippedColor(235, 199, 52);
	const Color PlayerInventoryMagicItemColor(69, 186, 190);
	const Color PlayerInventoryMagicItemEquippedColor(138, 255, 255);
	const Color PlayerInventoryUnequipableColor(199, 32, 0);

	const Color &getItemDisplayColor(const ItemInstance &itemInst);
}
