#ifndef CHARACTER_SHEET_UI_CONTROLLER_H
#define CHARACTER_SHEET_UI_CONTROLLER_H

class Game;
class ListBox;

struct InputActionCallbackValues;

namespace CharacterSheetUiController
{
	void onDoneButtonSelected(Game &game);
	void onDoneInputAction(const InputActionCallbackValues &values);

	void onNextPageButtonSelected(Game &game);
	
	void onBackToStatsButtonSelected(Game &game);
	void onBackToStatsInputAction(const InputActionCallbackValues &values);

	void onSpellbookButtonSelected(Game &game);
	void onDropButtonSelected(Game &game, int itemIndex);
	void onInventoryScrollDownButtonSelected(ListBox &listBox);
	void onInventoryScrollUpButtonSelected(ListBox &listBox);
}

#endif
