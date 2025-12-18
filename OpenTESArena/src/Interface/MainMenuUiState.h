#ifndef MAIN_MENU_UI_STATE_H
#define MAIN_MENU_UI_STATE_H

#include "../UI/UiButton.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct MainMenuUiState
{
	Game *game;

	// Test option state.
	int testType, testIndex, testIndex2, testWeather;

	UiContextElements elements;
	UiContextInputListeners inputListeners;

	MainMenuUiState();

	void init(Game &game);
};

namespace MainMenuUI
{
	DECLARE_UI_CONTEXT(MainMenu);

	void updateTypeTextBox();
	void updateNameTextBox();
	void updateWeatherTextBox();

	void onLoadGameButtonSelected(MouseButtonType mouseButtonType);
	void onNewGameButtonSelected(MouseButtonType mouseButtonType);
	void onExitGameButtonSelected(MouseButtonType mouseButtonType);
	void onTestGameButtonSelected(MouseButtonType mouseButtonType);
	void onTestTypeUpButtonSelected(MouseButtonType mouseButtonType);
	void onTestTypeDownButtonSelected(MouseButtonType mouseButtonType);
	void onTestIndexUpButtonSelected(MouseButtonType mouseButtonType);
	void onTestIndexDownButtonSelected(MouseButtonType mouseButtonType);
	void onTestIndex2UpButtonSelected(MouseButtonType mouseButtonType);
	void onTestIndex2DownButtonSelected(MouseButtonType mouseButtonType);
	void onTestWeatherUpButtonSelected(MouseButtonType mouseButtonType);
	void onTestWeatherDownButtonSelected(MouseButtonType mouseButtonType);

	void onLoadGameInputAction(const InputActionCallbackValues &values);
	void onNewGameInputAction(const InputActionCallbackValues &values);
	void onExitGameInputAction(const InputActionCallbackValues &values);
	void onTestGameInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(MainMenuUI, onLoadGameButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onNewGameButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onExitGameButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestGameButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestTypeUpButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestTypeDownButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestIndexUpButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestIndexDownButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestIndex2UpButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestIndex2DownButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestWeatherUpButtonSelected),
		DECLARE_UI_FUNC(MainMenuUI, onTestWeatherDownButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(MainMenuUI, onLoadGameInputAction),
		DECLARE_UI_FUNC(MainMenuUI, onNewGameInputAction),
		DECLARE_UI_FUNC(MainMenuUI, onExitGameInputAction),
		DECLARE_UI_FUNC(MainMenuUI, onTestGameInputAction)
	};
}

#endif
