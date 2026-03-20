#include "ImageSequencePanel.h"
#include "ImageSequenceUiState.h"
#include "../Game/Game.h"

ImageSequencePanel::ImageSequencePanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ImageSequenceUI::ContextName, game);
}

ImageSequencePanel::~ImageSequencePanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ImageSequenceUI::ContextName, game);
}

bool ImageSequencePanel::init()
{
	return true;
}
