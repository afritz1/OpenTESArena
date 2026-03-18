#include "InventoryUiView.h"
#include "../Items/ItemInstance.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextRenderUtils.h"

const Color &InventoryUiView::getItemDisplayColor(const ItemInstance &itemInst)
{
	// @todo magic items

	// @todo: class non-equippable items

	if (itemInst.isEquipped)
	{
		return InventoryUiView::PlayerInventoryEquipmentEquippedColor;
	}

	return InventoryUiView::PlayerInventoryEquipmentColor;
}
