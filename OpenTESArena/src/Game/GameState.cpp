#include <cassert>

#include "SDL.h"

#include "GameState.h"

#include "GameData.h"
#include "Options.h"
#include "OptionsParser.h"
#include "../Interface/Panel.h"
#include "../Math/Int2.h"
#include "../Media/AudioManager.h"
#include "../Media/MusicName.h"
#include "../Media/SoundName.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Media/WildMidi.hpp"
#include "../Rendering/CLProgram.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

const std::string GameState::DEFAULT_SCREEN_TITLE = "OpenTESArena";

GameState::GameState()
{
	Debug::mention("GameState", "Initializing.");

	// Load options from file.
	this->options = OptionsParser::parse();

	// Initialize virtual file system using the data path in the options file.
	VFS::Manager::get().initialize(std::string(this->options->getDataPath()));

	// Set the panel and music for the next tick. Don't use "this->panel" yet.
	this->nextPanel = Panel::defaultPanel(this);
	this->nextMusic = std::unique_ptr<MusicName>(new MusicName(
		MusicName::PercIntro));

	// Initialize the OpenAL Soft audio manager.
	this->audioManager.init(*this->options.get());

	// Initialize the SDL renderer and window with the given dimensions and title.
	this->renderer = std::unique_ptr<Renderer>(new Renderer(
		this->options->getScreenWidth(), this->options->getScreenHeight(),
		this->options->isFullscreen(), GameState::DEFAULT_SCREEN_TITLE));

	// Initialize the texture manager with the SDL window's pixel format.
	this->textureManager = std::unique_ptr<TextureManager>(new TextureManager(
		this->renderer->getRenderer(), this->renderer->getPixelFormat()));

	// Preload sequences, so that cinematic stuttering doesn't occur. It's because
	// cinematics otherwise load their frames one at a time while playing.
	// (Remove this functionality if actual videos will be used instead.)
	this->textureManager->preloadSequences();

	// Set window icon.
	this->renderer->setWindowIcon(TextureName::Icon, *this->textureManager.get());

	// Leave some things null for now. 
	this->gameData = nullptr;
	this->panel = nullptr;

	this->running = true;

	// Use a blitted surface as the cursor instead.
	SDL_ShowCursor(SDL_FALSE);

	// GameData is initialized when a player enters the game world. 
	// The panel is set at the beginning of a frame if one is waiting.
	assert(this->gameData.get() == nullptr);
	assert(this->panel.get() == nullptr);
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

AudioManager &GameState::getAudioManager()
{
	return this->audioManager;
}

GameData *GameState::getGameData() const
{
	return this->gameData.get();
}

Options &GameState::getOptions() const
{
	return *this->options.get();
}

Renderer &GameState::getRenderer() const
{
	return *this->renderer.get();
}

TextureManager &GameState::getTextureManager() const
{
	return *this->textureManager.get();
}

Int2 GameState::getScreenDimensions() const
{
	return this->renderer->getRenderDimensions();
}

SDL_Rect GameState::getLetterboxDimensions() const
{
	return this->renderer->getLetterboxDimensions();
}

void GameState::resizeWindow(int width, int height)
{
	this->renderer->resize(width, height);
	
	if (this->gameDataIsActive())
	{
		// Rebuild OpenCL program with new dimensions.		
		this->gameData->getCLProgram() = std::move(CLProgram(
			width, height, this->renderer->getRenderer(),
			this->gameData->getWorldWidth(),
			this->gameData->getWorldHeight(),
			this->gameData->getWorldDepth()));
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
		assert(this->nextPanel.get() == nullptr);
	}

	// Change the music if requested.
	if (this->nextMusic.get() != nullptr)
	{
		this->audioManager.playMusic(*this->nextMusic.get());
		this->nextMusic = nullptr;
	}

	// Tick the current panel by delta time (does nothing if not required).
	this->panel->tick(dt, this->running);
}

void GameState::render()
{
	// Now using GPU-accelerated renderer instead of SDL_Surface.
	auto *renderer = this->renderer->getRenderer();
	auto letterbox = this->getLetterboxDimensions();

	this->panel->render(renderer, &letterbox);

	this->renderer->present();
}
