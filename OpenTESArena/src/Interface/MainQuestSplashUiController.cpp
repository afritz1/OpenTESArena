#include "GameWorldPanel.h"
#include "MainQuestSplashUiController.h"
#include "../Audio/MusicLibrary.h"
#include "../Game/Game.h"

void MainQuestSplashUiController::onExitButtonSelected(Game &game)
{
	// Choose random dungeon music and enter game world.
	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
		MusicType::Interior, game.random, [](const MusicDefinition &def)
	{
		DebugAssert(def.type == MusicType::Interior);
		const InteriorMusicDefinition &interiorMusicDef = def.interior;
		return interiorMusicDef.type == InteriorMusicType::Dungeon;
	});

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing dungeon music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);

	game.setPanel<GameWorldPanel>();
}
