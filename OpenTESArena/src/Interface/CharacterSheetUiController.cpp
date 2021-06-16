#include "CharacterEquipmentPanel.h"
#include "CharacterPanel.h"
#include "CharacterSheetUiController.h"
#include "GameWorldPanel.h"
#include "../UI/ListBox.h"
#include "../Game/Game.h"

void CharacterSheetUiController::onDoneButtonSelected(Game &game)
{
	game.setPanel<GameWorldPanel>();
}

void CharacterSheetUiController::onNextPageButtonSelected(Game &game)
{
	game.setPanel<CharacterEquipmentPanel>();
}

void CharacterSheetUiController::onBackToStatsButtonSelected(Game &game)
{
	game.setPanel<CharacterPanel>();
}

void CharacterSheetUiController::onSpellbookButtonSelected(Game &game)
{
	// Nothing yet.
	// Might eventually take an argument for a panel?
}

void CharacterSheetUiController::onDropButtonSelected(Game &game, int itemIndex)
{
	// Nothing yet.
	// The index parameter will point to which item in the list to drop.
}

void CharacterSheetUiController::onInventoryScrollDownButtonSelected(ListBox &listBox)
{
	listBox.scrollDown();
}

void CharacterSheetUiController::onInventoryScrollUpButtonSelected(ListBox &listBox)
{
	listBox.scrollUp();
}
