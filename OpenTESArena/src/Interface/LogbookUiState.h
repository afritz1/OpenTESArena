#pragma once

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct LogbookUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	LogbookUiState();

	void init(Game &game);
};

namespace LogbookUI
{
	DECLARE_UI_CONTEXT(Logbook);

	void onBackButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(LogbookUI, onBackButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(LogbookUI, onBackInputAction)
	};
}
