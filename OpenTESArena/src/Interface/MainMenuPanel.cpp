#include "CommonUiView.h"
#include "MainMenuPanel.h"
#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "MainMenuUiState.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

MainMenuPanel::MainMenuPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(MainMenuUI::ContextType, game);
}

MainMenuPanel::~MainMenuPanel()
{
	Game &game = this->getGame();
	InputManager &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::MainMenu, false);

	// @todo this causes an error when exiting application because UiManager is destructed before MainMenuPanel
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(MainMenuUI::ContextType, game);
}

bool MainMenuPanel::init()
{
	Game &game = this->getGame();
	InputManager &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::MainMenu, true);

	// Unload in case we are returning from a game session.
	Renderer &renderer = game.renderer;
	SceneManager &sceneManager = game.sceneManager;
	sceneManager.renderVoxelChunkManager.unloadScene(renderer);
	sceneManager.renderEntityManager.unloadScene(renderer);

	return true;
}
