#ifndef INVENTORY_UI_MODEL_H
#define INVENTORY_UI_MODEL_H

#include <string>

#include "../Utilities/Color.h"

#include "components/utilities/Buffer.h"

class Game;

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

#endif
