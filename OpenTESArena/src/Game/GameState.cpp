#include <cassert>
#include <iostream>

#include <SDL2/SDL.h>

#include "GameState.h"

#include "GameData.h"
#include "Options.h"
#include "OptionsParser.h"
#include "../Interface/Panel.h"
#include "../Math/Int2.h"
#include "../Media/AudioManager.h"
#include "../Media/MusicFormat.h"
#include "../Media/MusicName.h"
#include "../Media/SoundFormat.h"
#include "../Media/SoundName.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/CLProgram.h"
#include "../Rendering/PostProcessing.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"


const std::string GameState::DEFAULT_SCREEN_TITLE = "OpenTESArena";

GameState::GameState()
{
	Debug::mention("GameState", "Initializing.");

	this->audioManager = nullptr;
	this->gameData = nullptr;
	this->nextMusic = nullptr;
	this->nextPanel = nullptr;
	this->options = nullptr;
	this->panel = nullptr;
	this->renderer = nullptr;
	this->textureManager = nullptr;

	// Load options from file.
	this->options = OptionsParser::parse();

    VFS::Manager::get().initialize(std::string(this->options->getDataPath()));

	// Not constructing the panel until the first tick guarantees that all
	// dependencies will be ready, but it doesn't matter anyway because there
	// shouldn't be dependencies with this new ordering.
	this->panel = nullptr;
	this->nextPanel = Panel::defaultPanel(this);
	this->nextMusic = std::unique_ptr<MusicName>(new MusicName(
		MusicName::PercIntro));

	// Initialize audio manager for MIDI music and Ogg sound with some channels.
	assert(this->options.get() != nullptr);
	this->audioManager = std::unique_ptr<AudioManager>(new AudioManager(
		this->options->getMusicFormat(), this->options->getSoundFormat(),
		this->options->getMusicVolume(), this->options->getSoundVolume(),
		this->options->getSoundChannelCount()));

	// Initialize the SDL renderer and window with the given dimensions and title.
	this->renderer = std::unique_ptr<Renderer>(new Renderer(
		this->options->getScreenWidth(), this->options->getScreenHeight(),
		this->options->isFullscreen(), GameState::DEFAULT_SCREEN_TITLE));

	// Initialize the texture manager with the SDL window's pixel format.
	this->textureManager = std::unique_ptr<TextureManager>(new TextureManager(
		this->renderer->getWindowSurface()->format));

	// Preload sequences, so that cinematic stuttering doesn't occur. It's because
	// cinematics otherwise load their frames one at a time while playing.
	assert(this->textureManager.get() != nullptr);
	this->textureManager->preloadSequences();

	// Set window icon.
	this->renderer->setWindowIcon(TextureName::Icon, *this->textureManager.get());

	this->running = true;

	// Use a blitted surface as the cursor instead.
	SDL_ShowCursor(SDL_FALSE);

	assert(this->audioManager.get() != nullptr);
	assert(this->gameData.get() == nullptr);
	assert(this->nextMusic.get() != nullptr);
	assert(this->options.get() != nullptr);
	assert(this->panel.get() == nullptr);
	assert(this->nextPanel.get() != nullptr);
	assert(this->renderer.get() != nullptr);
	assert(this->textureManager.get() != nullptr);
	assert(this->running == true);
}

GameState::~GameState()
{

}

bool GameState::isRunning() const
{
	return this->running;
}

bool GameState::gameDataIsActive() const
{
	return this->gameData.get() != nullptr;
}

AudioManager &GameState::getAudioManager() const
{
	return *this->audioManager.get();
}

GameData *GameState::getGameData() const
{
	return this->gameData.get();
}

Options &GameState::getOptions() const
{
	return *this->options.get();
}

TextureManager &GameState::getTextureManager() const
{
	return *this->textureManager.get();
}

Int2 GameState::getScreenDimensions() const
{
	return Int2(this->renderer->getWindowSurface()->w,
		this->renderer->getWindowSurface()->h);
}

std::unique_ptr<SDL_Rect> GameState::getLetterboxDimensions() const
{
	return this->renderer->getLetterboxDimensions();
}

void GameState::resizeWindow(int width, int height)
{
	this->renderer->resize(width, height);

	if (this->gameDataIsActive())
	{
		// Rebuild OpenCL program with new dimensions.
		this->gameData->getCLProgram() = CLProgram(width, height);
	}
}

void GameState::setPanel(std::unique_ptr<Panel> nextPanel)
{
	this->nextPanel = std::move(nextPanel);
}

void GameState::setMusic(MusicName name)
{
	this->nextMusic = std::unique_ptr<MusicName>(new MusicName(name));
}

void GameState::setGameData(std::unique_ptr<GameData> gameData)
{
	this->gameData = std::move(gameData);
}

void GameState::tick(double dt)
{
	// Change the panel if requested.
	if (this->nextPanel.get() != nullptr)
	{
		this->panel = std::move(this->nextPanel);
		this->nextPanel = nullptr;
	}

	// Change the music if requested.
	if (this->nextMusic.get() != nullptr)
	{
		this->audioManager->playMusic(*this->nextMusic.get());
		this->nextMusic = nullptr;
	}

	this->panel->tick(dt, this->running);
}

void GameState::render()
{
	auto *surface = this->renderer->getWindowSurface();
	auto letterbox = this->getLetterboxDimensions();

	this->panel->render(surface, letterbox.get());

	this->renderer->present();
}
