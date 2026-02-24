#include "PauseMenuPanel.h"
#include "PauseMenuUiState.h"
#include "../Game/Game.h"

PauseMenuPanel::PauseMenuPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(PauseMenuUI::ContextName, game);
}

PauseMenuPanel::~PauseMenuPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(PauseMenuUI::ContextName, game);
}

bool PauseMenuPanel::init()
{
	return true;
}
