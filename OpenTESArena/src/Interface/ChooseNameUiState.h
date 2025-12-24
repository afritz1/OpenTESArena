#ifndef CHOOSE_NAME_UI_STATE_H
#define CHOOSE_NAME_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct ChooseNameUiState
{
	Game *game;
	UiContextState contextState;

	ChooseNameUiState();

	void init(Game &game);
};

namespace ChooseNameUI
{
	DECLARE_UI_CONTEXT(ChooseName);

	void updateNameTextBox();

	void on_ButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseNameUI, on_ButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseNameUI, onBackInputAction)
	};
}

#endif
