#ifndef CHOOSE_RACE_UI_STATE_H
#define CHOOSE_RACE_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct ChooseRaceUiState
{
	Game *game;
	UiContextState contextState;

	ChooseRaceUiState();

	void init(Game &game);
};

namespace ChooseRaceUI
{
	DECLARE_UI_CONTEXT(ChooseRace);

	void on_ButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseRaceUI, on_ButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseRaceUI, onBackInputAction)
	};
}

#endif
