#include "ChooseAttributesPanel.h"
#include "ChooseAttributesUiState.h"
#include "../Game/Game.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game)
{

	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ChooseAttributesUI::ContextName, game);
}

ChooseAttributesPanel::~ChooseAttributesPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ChooseAttributesUI::ContextName, game);
}

bool ChooseAttributesPanel::init()
{
	return true;
}
