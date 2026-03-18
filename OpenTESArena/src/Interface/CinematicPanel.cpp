#include "CinematicPanel.h"
#include "CinematicUiState.h"
#include "../Game/Game.h"

CinematicPanel::CinematicPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(CinematicUI::ContextName, game);
}

CinematicPanel::~CinematicPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(CinematicUI::ContextName, game);
}

bool CinematicPanel::init()
{
	return true;
}
