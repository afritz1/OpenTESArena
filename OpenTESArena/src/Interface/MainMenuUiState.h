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

	UiContextElements elements;

	MainMenuUiState();

	void allocate(UiManager &uiManager, TextureManager &textureManager, Renderer &renderer);
	void free(UiManager &uiManager, Renderer &renderer);
};

namespace MainMenuUI
{
	static constexpr UiContextType ContextType = UiContextType::MainMenu;

	static MainMenuUiState state;

	void create(Game &game);
	void destroy(Game &game);
}

#endif
