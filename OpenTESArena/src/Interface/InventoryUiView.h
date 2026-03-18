#ifndef INVENTORY_UI_VIEW_H
#define INVENTORY_UI_VIEW_H

#include "../Math/Rect.h"
#include "../UI/ListBox.h"
#include "../Utilities/Color.h"

struct ItemInstance;

namespace InventoryUiView
{
	const Color PlayerInventoryEquipmentColor(211, 142, 0);
	const Color PlayerInventoryEquipmentEquippedColor(235, 199, 52);
	const Color PlayerInventoryMagicItemColor(69, 186, 190);
	const Color PlayerInventoryMagicItemEquippedColor(138, 255, 255);
	const Color PlayerInventoryUnequipableColor(199, 32, 0);

	const Color &getItemDisplayColor(const ItemInstance &itemInst);
}

#endif
