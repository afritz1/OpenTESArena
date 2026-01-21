#include "ChooseNamePanel.h"
#include "ChooseNameUiState.h"
#include "../Game/Game.h"

ChooseNamePanel::ChooseNamePanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ChooseNameUI::ContextName, game);
}

ChooseNamePanel::~ChooseNamePanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ChooseNameUI::ContextName, game);
}

bool ChooseNamePanel::init()
{
	return true;
}
