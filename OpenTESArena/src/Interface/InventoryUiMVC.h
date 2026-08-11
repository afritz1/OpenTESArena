#pragma once

#include <string>

#include "../Math/Rect.h"
#include "../Utilities/Color.h"

#include "components/utilities/Buffer.h"

class Game;
class ItemInventory;

struct CharacterClassDefinition;
struct ItemDefinition;
struct ItemInstance;
struct Player;

namespace InventoryUiModel
{
	std::string getItemText(int inventorySlotIndex, const ItemInventory &inventory);

	bool isItemEquippableByClass(const ItemDefinition &itemDef, const CharacterClassDefinition &charClassDef);
}

namespace InventoryUiView
{
	constexpr Color ItemDefaultColor(211, 142, 0);
	constexpr Color ItemEquippedColor(235, 199, 52);
	constexpr Color ItemMagicColor(69, 186, 190);
	constexpr Color ItemMagicEquippedColor(138, 255, 255);
	constexpr Color ItemUnequippableColor(199, 32, 0);

	Color getItemDisplayColor(int inventorySlotIndex, const Player &player);
}
