#ifndef MAIN_QUEST_SPLASH_UI_STATE_H
#define MAIN_QUEST_SPLASH_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct MainQuestSplashUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	int provinceID;

	MainQuestSplashUiState();

	void init(Game &game);
};

namespace MainQuestSplashUI
{
	DECLARE_UI_CONTEXT(MainQuestSplash);

	void onExitButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(MainQuestSplashUI, onExitButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(MainQuestSplashUI, onBackInputAction)
	};
}

#endif
