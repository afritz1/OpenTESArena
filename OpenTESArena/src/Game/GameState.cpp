#include <cassert>

#include "SDL.h"

#include "GameState.h"

#include "GameData.h"
#include "Options.h"
#include "OptionsParser.h"
#include "../Assets/TextAssets.h"
#include "../Interface/Panel.h"
#include "../Math/Int2.h"
#include "../Media/AudioManager.h"
#include "../Media/FontManager.h"
#include "../Media/MusicName.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/CLProgram.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

GameState::GameState()
{
	Debug::mention("GameState", "Initializing.");

	// Load options from file.
	this->options = OptionsParser::parse();

	// Initialize virtual file system using the Arena path in the options file.
	VFS::Manager::get().initialize(std::string(this->options->getArenaPath()));

	// Set the panel and music for the next tick. Don't use "this->panel" yet.
	this->nextPanel = Panel::defaultPanel(this);
	this->nextMusic = std::unique_ptr<MusicName>(new MusicName(
		MusicName::PercIntro));

	// Initialize the OpenAL Soft audio manager.
	this->audioManager.init(*this->options.get());

	// Initialize the SDL renderer and window with the given settings.
	this->renderer = std::unique_ptr<Renderer>(new Renderer(
		this->options->getScreenWidth(), this->options->getScreenHeight(),
		this->options->isFullscreen(), this->options->getLetterboxAspect()));

	// Initialize the texture manager with the SDL window's pixel format.
	this->textureManager = std::unique_ptr<TextureManager>(new TextureManager(
		*this->renderer.get()));

	// Initialize the font manager. Fonts (i.e., FONT_A.DAT) are loaded on demand.
	this->fontManager = std::unique_ptr<FontManager>(new FontManager(
		this->renderer->getFormat()));

	// Load various plain text assets.
	this->textAssets = std::unique_ptr<TextAssets>(new TextAssets());

	// Set window icon.
	this->renderer->setWindowIcon(TextureName::Icon, *this->textureManager.get());

	// Leave some things null for now. 
	this->gameData = nullptr;
	this->panel = nullptr;

	this->running = true;

	// Use a blitted surface as the cursor instead.
	SDL_ShowCursor(SDL_FALSE);

	// GameData is initialized when the player enters the game world. 
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

FontManager &GameState::getFontManager() const
{
	return *this->fontManager.get();
}

GameData *GameState::getGameData() const
{
	return this->gameData.get();
}

AudioManager &GameState::getAudioManager()
{
	return this->audioManager;
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

TextAssets &GameState::getTextAssets() const
{
	return *this->textAssets.get();
}

void GameState::resizeWindow(int width, int height)
{
	this->renderer->resize(width, height);
	
	if (this->gameDataIsActive())
	{
		// Rebuild OpenCL program with new dimensions.		
		this->gameData->getCLProgram() = std::move(CLProgram(
			this->gameData->getWorldWidth(),
			this->gameData->getWorldHeight(),
			this->gameData->getWorldDepth(),
			this->getTextureManager(),
			this->getRenderer(),
			this->getOptions().getRenderQuality()));
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
	}

	// Change the music if requested.
	if (this->nextMusic.get() != nullptr)
	{
		this->audioManager.playMusic(*this->nextMusic.get());
		this->nextMusic = nullptr;
	}

	// Tick the current panel by delta time.
	this->panel->tick(dt, this->running);
}

void GameState::render()
{
	this->panel->render(*this->renderer.get());
	this->renderer->present();
}
