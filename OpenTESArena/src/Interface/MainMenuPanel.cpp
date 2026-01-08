#include "MainMenuPanel.h"
#include "MainMenuUiState.h"
#include "../Game/Game.h"

MainMenuPanel::MainMenuPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(MainMenuUI::ContextType, game);
}

MainMenuPanel::~MainMenuPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(MainMenuUI::ContextType, game);
}

bool MainMenuPanel::init()
{
	return true;
}
