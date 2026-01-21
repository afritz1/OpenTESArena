#include "ChooseRacePanel.h"
#include "ChooseRaceUiState.h"
#include "../Game/Game.h"

ChooseRacePanel::ChooseRacePanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ChooseRaceUI::ContextName, game);
}

ChooseRacePanel::~ChooseRacePanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ChooseRaceUI::ContextName, game);
}

bool ChooseRacePanel::init()
{
	return true;
}
