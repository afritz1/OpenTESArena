#include "InventoryUiView.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"

ListBox::Properties InventoryUiView::makePlayerInventoryListBoxProperties(const FontLibrary &fontLibrary)
{
	const char *fontName = ArenaFontName::Teeny;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get player inventory list box font \"" + std::string(fontName) + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	constexpr double scrollScale = 1.0;
	constexpr int rowSpacing = 3;
	return ListBox::Properties(fontDefIndex, fontDef.getCharacterHeight(),
		InventoryUiView::PlayerInventoryEquipmentColor, scrollScale, rowSpacing);
}
