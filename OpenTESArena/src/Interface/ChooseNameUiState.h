#pragma once

#include <string>
#include <string_view>

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

struct ChooseNameUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	std::string name;

	ChooseNameUiState();

	void init(Game &game);
};

namespace ChooseNameUI
{
	DECLARE_UI_CONTEXT(ChooseName);

	void onTextInput(const std::string_view text);

	void onAcceptInputAction(const InputActionCallbackValues &values);
	void onBackspaceInputAction(const InputActionCallbackValues &values);
	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		{ "", UiButtonDefinitionCallback() }
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseNameUI, onAcceptInputAction),
		DECLARE_UI_FUNC(ChooseNameUI, onBackspaceInputAction),
		DECLARE_UI_FUNC(ChooseNameUI, onBackInputAction)
	};
}
