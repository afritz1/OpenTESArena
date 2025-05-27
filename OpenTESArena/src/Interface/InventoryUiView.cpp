#include "InventoryUiView.h"
#include "../Items/ItemInstance.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextRenderUtils.h"

ListBoxProperties InventoryUiView::makePlayerInventoryListBoxProperties(const FontLibrary &fontLibrary)
{
	const char *fontName = ArenaFontName::Teeny;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get player inventory list box font \"" + std::string(fontName) + "\".");
	}

	constexpr int maxDisplayedItemCount = 7;
	std::string dummyText;
	for (int i = 0; i < maxDisplayedItemCount; i++)
	{
		if (i > 0)
		{
			dummyText += '\n';
		}

		std::string dummyLine(24, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	constexpr int rowSpacing = 3;
	const TextRenderTextureGenInfo textureGenInfo =
		TextRenderUtils::makeTextureGenInfo(dummyText, fontDef, std::nullopt, rowSpacing);

	constexpr double scrollScale = 1.0;
	return ListBoxProperties(fontDefIndex, textureGenInfo, fontDef.getCharacterHeight(),
		InventoryUiView::PlayerInventoryEquipmentColor, scrollScale, rowSpacing);
}

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
