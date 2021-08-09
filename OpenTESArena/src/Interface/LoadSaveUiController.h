#ifndef LOAD_SAVE_UI_CONTROLLER_H
#define LOAD_SAVE_UI_CONTROLLER_H

class Game;

struct InputActionCallbackValues;

namespace LoadSaveUiController
{
	void onEntryButtonSelected(Game &game, int index);
	void onBackInputAction(const InputActionCallbackValues &values);
}

#endif
