#include "TextCinematicPanel.h"
#include "TextCinematicUiState.h"
#include "../Audio/AudioManager.h"
#include "../Game/Game.h"

TextCinematicPanel::TextCinematicPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(TextCinematicUI::ContextName, game);
}

TextCinematicPanel::~TextCinematicPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(TextCinematicUI::ContextName, game);

	// Stop voice if still playing.
	AudioManager &audioManager = game.audioManager;
	audioManager.stopSound();
}

bool TextCinematicPanel::init()
{
	return true;
}
