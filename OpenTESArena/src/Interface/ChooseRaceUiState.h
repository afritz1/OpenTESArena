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
	UiContextInstanceID contextInstID;
	UiContextInstanceID initialPopUpContextInstID;

	ChooseRaceUiState();

	void init(Game &game);
};

namespace ChooseRaceUI
{
	DECLARE_UI_CONTEXT(ChooseRace);

	void onMouseButtonChanged(Game &game, MouseButtonType mouseButtonType, const Int2 &position, bool pressed);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		{ "", UiButtonDefinitionCallback() }
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseRaceUI, onBackInputAction)
	};
}

#endif
