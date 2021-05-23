#ifndef CHARACTER_SHEET_UI_CONTROLLER_H
#define CHARACTER_SHEET_UI_CONTROLLER_H

class Game;
class ListBox;

namespace CharacterSheetUiController
{
	void onDoneButtonSelected(Game &game);
	void onNextPageButtonSelected(Game &game);
	void onBackToStatsButtonSelected(Game &game);
	void onSpellbookButtonSelected(Game &game);
	void onDropButtonSelected(Game &game, int itemIndex);
	void onInventoryScrollDownButtonSelected(ListBox &listBox);
	void onInventoryScrollUpButtonSelected(ListBox &listBox);
}

#endif
