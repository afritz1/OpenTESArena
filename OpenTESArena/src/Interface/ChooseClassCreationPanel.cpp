#include "ChooseClassCreationPanel.h"
#include "ChooseClassCreationUiState.h"
#include "../Game/Game.h"

ChooseClassCreationPanel::ChooseClassCreationPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ChooseClassCreationUI::ContextName, game);
}

ChooseClassCreationPanel::~ChooseClassCreationPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ChooseClassCreationUI::ContextName, game);
}

bool ChooseClassCreationPanel::init()
{
	return true;
}
