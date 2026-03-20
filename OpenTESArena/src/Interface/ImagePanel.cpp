#include "ImagePanel.h"
#include "ImageUiState.h"
#include "../Game/Game.h"

ImagePanel::ImagePanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ImageUI::ContextName, game);
}

ImagePanel::~ImagePanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ImageUI::ContextName, game);
}

bool ImagePanel::init()
{
	return true;
}
