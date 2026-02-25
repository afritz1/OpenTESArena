#include "WorldMapPanel.h"
#include "WorldMapUiState.h"
#include "../Game/Game.h"

WorldMapPanel::WorldMapPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(WorldMapUI::ContextName, game);
}

WorldMapPanel::~WorldMapPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(WorldMapUI::ContextName, game);
}

bool WorldMapPanel::init()
{
	return true;
}
