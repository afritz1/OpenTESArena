#pragma once

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"
#include "../WorldMap/ProvinceLibrary.h"

class Game;

enum class MouseButtonType;

struct MainQuestSplashUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	ProvinceDefinitionID provinceDefID;

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
