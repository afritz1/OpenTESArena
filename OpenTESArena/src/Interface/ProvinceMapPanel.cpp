#include "ProvinceMapPanel.h"
#include "ProvinceMapUiState.h"
#include "../Game/Game.h"

ProvinceMapPanel::ProvinceMapPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ProvinceMapUI::ContextName, game);
}

ProvinceMapPanel::~ProvinceMapPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ProvinceMapUI::ContextName, game);
}

bool ProvinceMapPanel::init()
{
	return true;
}
