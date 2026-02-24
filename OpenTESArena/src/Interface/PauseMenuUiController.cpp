#include "MainMenuPanel.h"
#include "PauseMenuUiController.h"
#include "../Audio/MusicLibrary.h"
#include "../Game/Game.h"

void PauseMenuUiController::onNewGameButtonSelected(Game &game)
{
	GameState &gameState = game.gameState;
	gameState.clearSession();

	game.setPanel<MainMenuPanel>();

	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(MusicType::MainMenu, game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing main menu music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);
}
