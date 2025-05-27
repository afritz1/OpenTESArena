#include "GameWorldPanel.h"
#include "MainQuestSplashUiController.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Game/Game.h"

void MainQuestSplashUiController::onExitButtonSelected(Game &game)
{
	// Choose random dungeon music and enter game world.
	const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing dungeon music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);

	game.setPanel<GameWorldPanel>();
}
