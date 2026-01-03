#include "ChooseNamePanel.h"
#include "ChooseNameUiState.h"
#include "../Game/Game.h"

ChooseNamePanel::ChooseNamePanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ChooseNameUI::ContextType, game);
}

ChooseNamePanel::~ChooseNamePanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ChooseNameUI::ContextType, game);
}

bool ChooseNamePanel::init()
{
	return true;
}
