#ifndef CHOOSE_CLASS_CREATION_UI_STATE_H
#define CHOOSE_CLASS_CREATION_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct ChooseClassCreationUiState
{
	Game *game;
	UiContextState contextState;

	ChooseClassCreationUiState();

	void init(Game &game);
};

namespace ChooseClassCreationUI
{
	DECLARE_UI_CONTEXT(ChooseClassCreation);

	void onGenerateButtonSelected(MouseButtonType mouseButtonType);
	void onSelectButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseClassCreationUI, onGenerateButtonSelected),
		DECLARE_UI_FUNC(ChooseClassCreationUI, onSelectButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseClassCreationUI, onBackInputAction)
	};
}

#endif
