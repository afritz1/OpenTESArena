#include "ChooseClassPanel.h"
#include "ChooseClassUiState.h"
#include "../Game/Game.h"

ChooseClassPanel::ChooseClassPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ChooseClassUI::ContextName, game);
}

ChooseClassPanel::~ChooseClassPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ChooseClassUI::ContextName, game);
}

bool ChooseClassPanel::init()
{
	return true;
}
