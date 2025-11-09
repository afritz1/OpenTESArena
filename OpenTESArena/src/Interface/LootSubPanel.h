#ifndef LOOT_SUB_PANEL_H
#define LOOT_SUB_PANEL_H

#include <functional>

#include "Panel.h"
#include "../UI/ListBox.h"
#include "../UI/TextBox.h"

class ItemInventory;

// For keeping loot list box callbacks valid when removing inventory items.
struct LootUiItemMapping
{
	int inventoryItemIndex;
	int listBoxItemIndex;
};

// Displays items to transfer to player inventory.
class LootSubPanel : public Panel
{
public:
	using OnClosedFunction = std::function<void(Game&)>; // In case entity inventory becomes empty.
private:
	ListBox listBox;
	Button<Game&> closeButton;
	ScopedUiTextureRef textureRef, cursorTextureRef;
	std::vector<LootUiItemMapping> itemMappings;
public:
	LootSubPanel(Game &game);
	~LootSubPanel() override = default;

	bool init(ItemInventory &itemInventory, const OnClosedFunction &onClosed);
};

#endif
