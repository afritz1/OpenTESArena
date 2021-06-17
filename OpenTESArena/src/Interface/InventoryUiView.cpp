#include "InventoryUiView.h"
#include "../UI/FontLibrary.h"
#include "../UI/FontName.h"
#include "../UI/FontUtils.h"

ListBox::Properties InventoryUiView::makePlayerInventoryListBoxProperties(const FontLibrary &fontLibrary)
{
	const char *fontNameStr = FontUtils::fromName(FontName::Teeny);
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontNameStr, &fontDefIndex))
	{
		DebugCrash("Couldn't get player inventory list box font \"" + std::string(fontNameStr) + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	constexpr double scrollScale = 1.0;
	constexpr int rowSpacing = 3;
	return ListBox::Properties(fontDefIndex, fontDef.getCharacterHeight(),
		InventoryUiView::PlayerInventoryEquipmentColor, scrollScale, rowSpacing);
}
