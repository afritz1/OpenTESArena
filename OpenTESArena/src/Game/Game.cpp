#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>

#include "SDL.h"

#include "Game.h"

#include "GameData.h"
#include "Options.h"
#include "OptionsParser.h"
#include "../Assets/TextAssets.h"
#include "../Interface/Panel.h"
#include "../Math/Int2.h"
#include "../Media/AudioManager.h"
#include "../Media/FontManager.h"
#include "../Media/MusicFile.h"
#include "../Media/MusicName.h"
#include "../Media/PPMFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

Game::Game()
{
	Debug::mention("Game", "Initializing (Platform: " +
		std::string(SDL_GetPlatform()) + ").");

	// Load options from file.
	this->options = OptionsParser::parse();

	// Initialize virtual file system using the Arena path in the options file.
	VFS::Manager::get().initialize(std::string(this->options->getArenaPath()));

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
	this->fontManager = std::unique_ptr<FontManager>(new FontManager());

	// Load various plain text assets.
	this->textAssets = std::unique_ptr<TextAssets>(new TextAssets());

	// Set window icon.
	int iconWidth, iconHeight;
	auto iconPixels = PPMFile::read("data/icon.ppm", iconWidth, iconHeight);
	SDL_Surface *icon = Surface::createSurfaceWithFormatFrom(iconPixels.get(),
		iconWidth, iconHeight, Renderer::DEFAULT_BPP,
		iconWidth * sizeof(*iconPixels.get()), Renderer::DEFAULT_PIXELFORMAT);
	this->renderer->setWindowIcon(icon);
	SDL_FreeSurface(icon);

	// Initialize panel and music to default.
	this->panel = Panel::defaultPanel(this);
	this->setMusic(MusicName::PercIntro);

	// Use a texture as the cursor instead.
	SDL_ShowCursor(SDL_FALSE);

	// Leave some members null for now. The game data is initialized when the player 
	// enters the game world, and the "next panel" is a temporary used by the game
	// to avoid corruption between panel events which change the panel.
	this->gameData = nullptr;
	this->nextPanel = nullptr;
}

Game::~Game()
{

}

AudioManager &Game::getAudioManager()
{
	return this->audioManager;
}

FontManager &Game::getFontManager() const
{
	return *this->fontManager.get();
}

bool Game::gameDataIsActive() const
{
	return this->gameData.get() != nullptr;
}

GameData &Game::getGameData() const
{
	// The caller should not request the game data when there is no active session.
	assert(this->gameDataIsActive());

	return *this->gameData.get();
}

Options &Game::getOptions() const
{
	return *this->options.get();
}

Renderer &Game::getRenderer() const
{
	return *this->renderer.get();
}

TextureManager &Game::getTextureManager() const
{
	return *this->textureManager.get();
}

TextAssets &Game::getTextAssets() const
{
	return *this->textAssets.get();
}

void Game::setPanel(std::unique_ptr<Panel> nextPanel)
{
	this->nextPanel = std::move(nextPanel);
}

void Game::setMusic(MusicName name)
{
	const std::string &filename = MusicFile::fromName(name);
	this->audioManager.playMusic(filename);
}

void Game::setGameData(std::unique_ptr<GameData> gameData)
{
	this->gameData = std::move(gameData);
}

void Game::delay(int ms)
{
	assert(ms >= 0);
	SDL_Delay(static_cast<uint32_t>(ms));
}

void Game::resizeWindow(int width, int height)
{
	// Resize the window, and the 3D renderer if initialized.
	this->renderer->resize(width, height, this->getOptions().getResolutionScale());
}

void Game::handleEvents(bool &running)
{
	// Handle events for the current game state.
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		// Application events and window resizes are handled here.
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool takeScreenshot = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_PRINTSCREEN);

		if (applicationExit)
		{
			running = false;
		}

		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->resizeWindow(width, height);
		}

		if (takeScreenshot)
		{
			// Save a screenshot to the local folder.
			auto &renderer = this->getRenderer();
			Surface screenshot(renderer.getScreenshot());
			SDL_SaveBMP(screenshot.get(), "out.bmp");
		}

		// Panel-specific events are handled by the panel.
		this->panel->handleEvent(e);

		// If the panel event requested a new panel, switch to it and send the
		// remaining events for this frame to the new panel.
		if (this->nextPanel.get() != nullptr)
		{
			this->panel = std::move(this->nextPanel);
		}
	}
}

void Game::tick(double dt)
{
	// Tick the current panel by delta time.
	this->panel->tick(dt);

	// If the panel tick requested a new panel, switch to it.
	if (this->nextPanel.get() != nullptr)
	{
		this->panel = std::move(this->nextPanel);
	}
}

void Game::render()
{
	this->panel->render(*this->renderer.get());
	this->renderer->present();
}

void Game::loop()
{
	// Longest allowed frame time.
	const int maximumMS = 1000 / Options::MIN_FPS;

	int thisTime = SDL_GetTicks();
	int lastTime = thisTime;

	// Primary game loop.
	bool running = true;
	while (running)
	{
		lastTime = thisTime;
		thisTime = SDL_GetTicks();

		// Fastest allowed frame time in milliseconds.
		const int minimumMS = 1000 / this->options->getTargetFPS();

		// Delay the current frame if the previous one was too fast.
		int frameTime = thisTime - lastTime;
		if (frameTime < minimumMS)
		{
			this->delay(minimumMS - frameTime);
			thisTime = SDL_GetTicks();
			frameTime = thisTime - lastTime;
		}

		// Clamp the delta time to at most the maximum frame time.
		const double dt = std::fmin(frameTime, maximumMS) / 1000.0;

		// Listen for input events.
		this->handleEvents(running);

		// Animate the current game state by delta time.
		this->tick(dt);

		// Draw to the screen.
		this->render();
	}
}
