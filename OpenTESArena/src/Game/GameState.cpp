#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "GameState.h"

#include "../Interface/Panel.h"
#include "../Media/AudioManager.h"
#include "../Media/MusicFormat.h"
#include "../Media/MusicName.h"
#include "../Media/SoundFormat.h"
#include "../Media/SoundName.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/PostProcessing.h"
#include "../Rendering/Renderer.h"

const int GameState::DEFAULT_SCREEN_WIDTH = 1280;
const int GameState::DEFAULT_SCREEN_HEIGHT = 720;
const bool GameState::DEFAULT_IS_FULLSCREEN = false;
const std::string GameState::DEFAULT_SCREEN_TITLE = "OpenTESArena";

GameState::GameState()
{
	this->audioManager = nullptr;
	this->nextMusic = nullptr;
	this->nextPanel = nullptr;
	this->panel = nullptr;
	this->renderer = nullptr;
	this->textureManager = nullptr;

	// Not constructing the panel until the first tick guarantees that all
	// dependencies will be ready, but it doesn't matter anyway because there
	// shouldn't be dependencies with this new ordering.
	this->panel = nullptr;
	this->nextPanel = Panel::defaultPanel(this);
	this->nextMusic = std::unique_ptr<MusicName>(new MusicName(
		MusicName::PercIntro));

	this->audioManager = std::unique_ptr<AudioManager>(new AudioManager(
		MusicFormat::MIDI, SoundFormat::Ogg, 64));
	this->renderer = std::unique_ptr<Renderer>(new Renderer(
		GameState::DEFAULT_SCREEN_WIDTH, GameState::DEFAULT_SCREEN_HEIGHT,
		GameState::DEFAULT_IS_FULLSCREEN, GameState::DEFAULT_SCREEN_TITLE));
	this->textureManager = std::unique_ptr<TextureManager>(new TextureManager(
		this->renderer->getWindowSurface()->format));

	// Set window icon.
	assert(this->textureManager.get() != nullptr);
	this->renderer->setWindowIcon(TextureName::Icon, *this->textureManager.get());

	this->running = true;

	SDL_ShowCursor(SDL_FALSE);

	assert(this->audioManager.get() != nullptr);
	assert(this->nextMusic.get() != nullptr);
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

AudioManager &GameState::getAudioManager() const
{
	return *this->audioManager.get();
}

TextureManager &GameState::getTextureManager() const
{
	return *this->textureManager.get();
}

std::unique_ptr<SDL_Rect> GameState::getLetterboxDimensions() const
{
	return this->renderer->getLetterboxDimensions();
}

void GameState::resizeWindow(int width, int height)
{
	this->renderer->resize(width, height);
}

void GameState::setPanel(std::unique_ptr<Panel> nextPanel)
{
	this->nextPanel = std::move(nextPanel);
}

void GameState::setMusic(MusicName name)
{
	this->nextMusic = std::unique_ptr<MusicName>(new MusicName(name));
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

	// Post processing optionally goes here.

	this->renderer->present();
}
