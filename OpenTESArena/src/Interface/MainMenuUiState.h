#ifndef MAIN_MENU_UI_STATE_H
#define MAIN_MENU_UI_STATE_H

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"

class Game;
class TextureManager;

struct MainMenuUiState
{
	UiTextureID bgTextureID;

	// Test option state.
	UiTextureID testArrowsTextureID;
	UiTextureID testButtonTextureID;
	int testType, testIndex, testIndex2, testWeather;

	// Elements with hotkeys.
	UiElementInstanceID loadGameButtonElementInstID, newGameButtonElementInstID, exitGameButtonElementInstID, testGameButtonElementInstID;

	// Conditionally enabled test elements.
	UiElementInstanceID testIndex2ImageElementInstID, testIndex2UpButtonElementInstID, testIndex2DownButtonElementInstID;
	UiElementInstanceID testWeatherImageElementInstID, testWeatherTextBoxElementInstID, testWeatherUpButtonElementInstID, testWeatherDownButtonElementInstID;

	UiContextElements elements;
	UiContextInputListeners inputListeners;

	MainMenuUiState();

	void allocate(UiManager &uiManager, TextureManager &textureManager, Renderer &renderer);
	void free(UiManager &uiManager, InputManager &inputManager, Renderer &renderer);
};

namespace MainMenuUI
{
	static constexpr UiContextType ContextType = UiContextType::MainMenu;

	static MainMenuUiState state;

	void create(Game &game);
	void destroy(Game &game);
	void update(double dt, Game &game);
}

#endif
