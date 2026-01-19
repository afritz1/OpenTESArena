#include "ChooseRacePanel.h"
#include "ChooseRaceUiState.h"
#include "../Game/Game.h"

ChooseRacePanel::ChooseRacePanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ChooseRaceUI::ContextType, game);
}

ChooseRacePanel::~ChooseRacePanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ChooseRaceUI::ContextType, game);
}

bool ChooseRacePanel::init()
{
	return true;
}
