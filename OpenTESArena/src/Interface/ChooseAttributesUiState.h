#ifndef CHOOSE_ATTRIBUTES_UI_STATE_H
#define CHOOSE_ATTRIBUTES_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct ChooseAttributesUiState
{
	Game *game;
	UiContextState contextState;

	ChooseAttributesUiState();

	void init(Game &game);
};

namespace ChooseAttributesUI
{
	DECLARE_UI_CONTEXT(ChooseAttributes);

	void on_ButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseAttributesUI, on_ButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseAttributesUI, onBackInputAction)
	};
}

#endif
