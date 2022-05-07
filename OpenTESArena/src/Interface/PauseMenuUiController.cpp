#include <algorithm>

#include "SDL.h"

#include "GameWorldPanel.h"
#include "LoadSavePanel.h"
#include "MainMenuPanel.h"
#include "OptionsPanel.h"
#include "PauseMenuPanel.h"
#include "PauseMenuUiController.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"

void PauseMenuUiController::onNewGameButtonSelected(Game &game)
{
	game.getGameState().clearSession();
	game.setPanel<MainMenuPanel>();

	const MusicLibrary &musicLibrary = game.getMusicLibrary();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
		MusicDefinition::Type::MainMenu, game.getRandom());

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing main menu music.");
	}

	AudioManager &audioManager = game.getAudioManager();
	audioManager.setMusic(musicDef);
}

void PauseMenuUiController::onLoadButtonSelected(Game &game)
{
	game.setPanel<LoadSavePanel>(LoadSavePanel::Type::Load);
}

void PauseMenuUiController::onSaveButtonSelected(Game &game)
{
	// @todo

	// SaveGamePanel...
	//auto optionsPanel = std::make_unique<OptionsPanel>(game);
	//game.setPanel(std::move(optionsPanel));
}

void PauseMenuUiController::onExitButtonSelected(Game &game)
{
	SDL_Event evt;
	evt.quit.type = SDL_QUIT;
	evt.quit.timestamp = 0;
	SDL_PushEvent(&evt);
}

void PauseMenuUiController::onResumeButtonSelected(Game &game)
{
	game.setPanel<GameWorldPanel>();
}

void PauseMenuUiController::onOptionsButtonSelected(Game &game)
{
	game.setPanel<OptionsPanel>();
}

void PauseMenuUiController::onSoundUpButtonSelected(Game &game, PauseMenuPanel &panel)
{
	auto &options = game.getOptions();
	options.setAudio_SoundVolume(std::min(options.getAudio_SoundVolume() + 0.050, 1.0));

	auto &audioManager = game.getAudioManager();
	audioManager.setSoundVolume(options.getAudio_SoundVolume());
	panel.updateSoundText(options.getAudio_SoundVolume());
}

void PauseMenuUiController::onSoundDownButtonSelected(Game &game, PauseMenuPanel &panel)
{
	auto &options = game.getOptions();
	const double newVolume = [&options]()
	{
		const double volume = std::max(options.getAudio_SoundVolume() - 0.050, 0.0);

		// Clamp very small values to zero to avoid precision issues with tiny numbers.
		return volume < Constants::Epsilon ? 0.0 : volume;
	}();

	options.setAudio_SoundVolume(newVolume);

	auto &audioManager = game.getAudioManager();
	audioManager.setSoundVolume(options.getAudio_SoundVolume());
	panel.updateSoundText(options.getAudio_SoundVolume());
}

void PauseMenuUiController::onMusicUpButtonSelected(Game &game, PauseMenuPanel &panel)
{
	auto &options = game.getOptions();
	options.setAudio_MusicVolume(std::min(options.getAudio_MusicVolume() + 0.050, 1.0));

	auto &audioManager = game.getAudioManager();
	audioManager.setMusicVolume(options.getAudio_MusicVolume());
	panel.updateMusicText(options.getAudio_MusicVolume());
}

void PauseMenuUiController::onMusicDownButtonSelected(Game &game, PauseMenuPanel &panel)
{
	auto &options = game.getOptions();
	const double newVolume = [&options]()
	{
		const double volume = std::max(options.getAudio_MusicVolume() - 0.050, 0.0);

		// Clamp very small values to zero to avoid precision issues with tiny numbers.
		return volume < Constants::Epsilon ? 0.0 : volume;
	}();

	options.setAudio_MusicVolume(newVolume);

	auto &audioManager = game.getAudioManager();
	audioManager.setMusicVolume(options.getAudio_MusicVolume());
	panel.updateMusicText(options.getAudio_MusicVolume());
}
