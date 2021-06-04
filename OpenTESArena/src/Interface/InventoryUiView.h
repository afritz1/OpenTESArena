#ifndef INVENTORY_UI_VIEW_H
#define INVENTORY_UI_VIEW_H

#include "../Media/Color.h"
#include "../UI/FontName.h"

namespace InventoryUiView
{
	constexpr int PlayerInventoryListBoxX = 14;
	constexpr int PlayerInventoryListBoxY = 50;
	constexpr FontName PlayerInventoryListBoxFontName = FontName::Teeny;

	constexpr int PlayerInventoryMaxDisplayedItems = 7;
	constexpr int PlayerInventoryRowSpacing = 3;

	const Color PlayerInventoryEquipmentColor(211, 142, 0);
	const Color PlayerInventoryEquipmentEquippedColor(235, 199, 52);
	const Color PlayerInventoryMagicItemColor(69, 186, 190);
	const Color PlayerInventoryMagicItemEquippedColor(138, 255, 255);
	const Color PlayerInventoryUnequipableColor(199, 32, 0);
}

#endif
