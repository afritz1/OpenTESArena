#ifndef CHOOSE_CLASS_UI_STATE_H
#define CHOOSE_CLASS_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct ChooseClassUiState
{
	Game *game;
	UiContextState contextState;

	ChooseClassUiState();

	void init(Game &game);
};

namespace ChooseClassUI
{
	DECLARE_UI_CONTEXT(ChooseClass);

	// @todo populate listbox via getElementByName() after createContext()

	// @todo listbox support, up/down buttons, item select button

	void on_ButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseClassUI, on_ButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseClassUI, onBackInputAction)
	};
}

#endif
