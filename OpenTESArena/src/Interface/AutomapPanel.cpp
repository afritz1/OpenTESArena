#include "AutomapPanel.h"
#include "AutomapUiState.h"
#include "../Game/Game.h"

AutomapPanel::AutomapPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(AutomapUI::ContextType, game);
}

AutomapPanel::~AutomapPanel()
{
	Game &game = this->getGame();	
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(AutomapUI::ContextType, game);
}

bool AutomapPanel::init()
{
	return true;
}
