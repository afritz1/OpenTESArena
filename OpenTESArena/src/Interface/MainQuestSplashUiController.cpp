#include "GameWorldPanel.h"
#include "MainQuestSplashUiController.h"
#include "../Audio/MusicLibrary.h"
#include "../Game/Game.h"

void MainQuestSplashUiController::onExitButtonSelected(Game &game)
{
	// Choose random dungeon music and enter game world.
	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
		MusicDefinition::Type::Interior, game.getRandom(), [](const MusicDefinition &def)
	{
		DebugAssert(def.getType() == MusicDefinition::Type::Interior);
		const auto &interiorMusicDef = def.getInteriorMusicDefinition();
		return interiorMusicDef.type == MusicDefinition::InteriorMusicDefinition::Type::Dungeon;
	});

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing dungeon music.");
	}

	AudioManager &audioManager = game.getAudioManager();
	audioManager.setMusic(musicDef);

	game.setPanel<GameWorldPanel>();
}
