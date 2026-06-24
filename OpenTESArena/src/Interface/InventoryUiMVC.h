#pragma once

#include <string>

#include "../Math/Rect.h"
#include "../Utilities/Color.h"

#include "components/utilities/Buffer.h"

class Game;

struct ItemInstance;
struct Player;

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
	constexpr Color ItemDefaultColor(211, 142, 0);
	constexpr Color ItemEquippedColor(235, 199, 52);
	constexpr Color ItemMagicColor(69, 186, 190);
	constexpr Color ItemMagicEquippedColor(138, 255, 255);
	constexpr Color ItemUnequippableColor(199, 32, 0);

	Color getItemDisplayColor(const ItemInstance &itemInst, const Player &player);
}
