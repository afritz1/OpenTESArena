#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <thread>

#include "SDL.h"

#include "Game.h"
#include "GameData.h"
#include "Options.h"
#include "PlayerInterface.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/TextAssets.h"
#include "../Interface/Panel.h"
#include "../Media/FontManager.h"
#include "../Media/MusicFile.h"
#include "../Media/MusicName.h"
#include "../Media/PPMFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/Platform.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

Game::Game()
{
	DebugMention("Initializing (Platform: " + std::string(SDL_GetPlatform()) + ").");

	// Get the current working directory. This is most relevant for platforms
	// like macOS, where the base path might be in the app's own "Resources" folder.
	this->basePath = Platform::getBasePath();

	// Get the path to the options folder. This is platform-dependent and points inside 
	// the "preferences directory" so it's always writable. Append "options.txt" to access
	// the file itself.
	this->optionsPath = Platform::getOptionsPath();

	// Parse options-default.txt. Always prefer the "default" file before the "changes" file. 
	// The changes file is stored in the user's prefs folder.
	this->options = [this]()
	{
		std::unique_ptr<Options> newOptions(new Options());

		const std::string changesOptionsPath(this->optionsPath + Options::CHANGES_FILENAME);
		const bool changesOptionsExists = File::exists(changesOptionsPath);

		if (!changesOptionsExists)
		{
			// If the "changes" options file doesn't exist, make one. Since the new options 
			// object has no changes, the new file will have no key-value pairs.
			DebugMention("Creating options file at \"" + changesOptionsPath + "\".");
			newOptions->saveChanges();
		}
		else
		{
			// Read in any key-value pairs in the "changes" options file.
			newOptions->load(changesOptionsPath);
		}

		return newOptions;
	}();

	// Verify that GLOBAL.BSA (the most important Arena file) exists.
	const bool arenaPathIsRelative = File::pathIsRelative(
		this->getOptions().getArenaPath());
	const std::string globalBsaPath = [this, arenaPathIsRelative]()
	{
		// Include the base path if the ArenaPath is relative.
		return (arenaPathIsRelative ? this->basePath : "") +
			this->options->getArenaPath() + "/GLOBAL.BSA";
	}();

	DebugAssert(File::exists(globalBsaPath),
		"\"" + this->options->getArenaPath() + "\" not a valid ARENA path.");

	// Initialize virtual file system using the Arena path in the options file.	
	VFS::Manager::get().initialize(std::string(
		(arenaPathIsRelative ? this->basePath : "") + this->options->getArenaPath()));

	// Initialize the OpenAL Soft audio manager.
	this->audioManager.init(*this->options.get());

	// Initialize the SDL renderer and window with the given settings.
	this->renderer = std::unique_ptr<Renderer>(new Renderer(
		this->options->getScreenWidth(), this->options->getScreenHeight(),
		this->options->getFullscreen(), this->options->getLetterboxAspect()));

	// Initialize the texture manager with the SDL window's pixel format.
	this->textureManager = std::unique_ptr<TextureManager>(new TextureManager(
		*this->renderer.get()));

	// Initialize the font manager. Fonts (i.e., FONT_A.DAT) are loaded on demand.
	this->fontManager = std::unique_ptr<FontManager>(new FontManager());

	// Load various plain text assets.
	this->textAssets = std::unique_ptr<TextAssets>(new TextAssets());

	// Load city data file.
	this->cityDataFile = std::unique_ptr<CityDataFile>(new CityDataFile("CITYDATA.00"));

	// Set window icon (treat black as transparent for 24-bit PPMs).
	int iconWidth, iconHeight;
	std::unique_ptr<uint32_t[]> iconPixels = PPMFile::read(
		this->basePath + "data/icon.ppm", iconWidth, iconHeight);
	Surface icon(Surface::createSurfaceWithFormatFrom(iconPixels.get(),
		iconWidth, iconHeight, Renderer::DEFAULT_BPP,
		iconWidth * sizeof(*iconPixels.get()), Renderer::DEFAULT_PIXELFORMAT));
	SDL_SetColorKey(icon.get(), SDL_TRUE, SDL_MapRGBA(icon.get()->format, 0, 0, 0, 255));
	this->renderer->setWindowIcon(icon.get());

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
	this->nextSubPanel = nullptr;

	// This keeps the programmer from deleting a sub-panel the same frame it's in use.
	// The pop is delayed until the beginning of the next frame.
	this->requestedSubPanelPop = false;
}

Game::~Game()
{

}

AudioManager &Game::getAudioManager()
{
	return this->audioManager;
}

const InputManager &Game::getInputManager() const
{
	return this->inputManager;
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

CityDataFile &Game::getCityDataFile() const
{
	return *this->cityDataFile.get();
}

const FPSCounter &Game::getFPSCounter() const
{
	return this->fpsCounter;
}

void Game::setPanel(std::unique_ptr<Panel> nextPanel)
{
	this->nextPanel = std::move(nextPanel);
}

void Game::pushSubPanel(std::unique_ptr<Panel> nextSubPanel)
{
	this->nextSubPanel = std::move(nextSubPanel);
}

void Game::popSubPanel()
{
	// The active sub-panel must not pop more than one sub-panel, because it may 
	// have unintended side effects for other panels below it.
	DebugAssert(!this->requestedSubPanelPop, "Already scheduled to pop this sub-panel.");

	// If there are no sub-panels, then there is only the main panel, and panels 
	// should never have any sub-panels to pop.
	DebugAssert(this->subPanels.size() > 0, "No sub-panels to pop.");

	this->requestedSubPanelPop = true;
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

void Game::resizeWindow(int width, int height)
{
	// Resize the window, and the 3D renderer if initialized.
	auto &options = this->getOptions();
	const bool fullGameWindow = options.getModernInterface();
	this->renderer->resize(width, height, options.getResolutionScale(), fullGameWindow);
}

void Game::handlePanelChanges()
{
	// If a sub-panel pop was requested, then pop the top of the sub-panel stack.
	if (this->requestedSubPanelPop)
	{
		this->subPanels.pop_back();
		this->requestedSubPanelPop = false;
	}

	// If a new sub-panel was requested, then add it to the stack.
	if (this->nextSubPanel.get() != nullptr)
	{
		this->subPanels.push_back(std::move(this->nextSubPanel));
	}

	// If a new panel was requested, switch to it. If it will be the active panel 
	// (i.e., there are no sub-panels), then subsequent events will be sent to it.
	if (this->nextPanel.get() != nullptr)
	{
		this->panel = std::move(this->nextPanel);
	}
}

void Game::handleEvents(bool &running)
{
	// Handle events for the current game state.
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		// Application events and window resizes are handled here.
		bool applicationExit = this->inputManager.applicationExit(e);
		bool resized = this->inputManager.windowResized(e);
		bool takeScreenshot = this->inputManager.keyPressed(e, SDLK_PRINTSCREEN);

		if (applicationExit)
		{
			running = false;
		}

		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->resizeWindow(width, height);

			// Call each panel's resize method. The panels should not be listening for
			// resize events themselves because it's more of an "application event" than
			// a panel event.
			this->panel->resize(width, height);

			for (auto &subPanel : this->subPanels)
			{
				subPanel->resize(width, height);
			}
		}

		if (takeScreenshot)
		{
			// Save a screenshot to the local folder.
			auto &renderer = this->getRenderer();
			Surface screenshot(renderer.getScreenshot());
			SDL_SaveBMP(screenshot.get(), "out.bmp");
		}

		// Panel-specific events are handled by the active panel or sub-panel. If any 
		// sub-panels exist, choose the top one. Otherwise, choose the main panel.
		if (this->subPanels.size() > 0)
		{
			this->subPanels.back()->handleEvent(e);
		}
		else
		{
			this->panel->handleEvent(e);
		}

		// See if the event requested any changes in active panels.
		this->handlePanelChanges();
	}
}

void Game::tick(double dt)
{
	// If any sub-panels are active, tick the top one by delta time. Otherwise, 
	// tick the main panel.
	if (this->subPanels.size() > 0)
	{
		this->subPanels.back()->tick(dt);
	}
	else
	{
		this->panel->tick(dt);
	}

	// See if the panel tick requested any changes in active panels.
	this->handlePanelChanges();
}

void Game::render()
{
	// Draw the panel's main content.
	this->panel->render(*this->renderer.get());

	// Draw any sub-panels back to front.
	for (auto &subPanel : this->subPanels)
	{
		subPanel->render(*this->renderer.get());
	}

	// Get the active panel's cursor texture and alignment.
	const std::pair<SDL_Texture*, CursorAlignment> cursor = (this->subPanels.size() > 0) ?
		this->subPanels.back()->getCurrentCursor() : this->panel->getCurrentCursor();

	// Draw cursor if not null. Some panels do not define a cursor (like cinematics), 
	// so their cursor is always null.
	if (cursor.first != nullptr)
	{
		// The panel should not be drawing the cursor themselves. It's done here 
		// just to make sure that the cursor is drawn only once and is always drawn last.
		this->renderer->drawCursor(cursor.first, cursor.second,
			this->inputManager.getMousePosition(), this->options->getCursorScale());
	}

	this->renderer->present();
}

void Game::loop()
{
	// Longest allowed frame time in microseconds.
	const std::chrono::duration<int64_t, std::micro> maximumMS(1000000 / Options::MIN_FPS);

	auto thisTime = std::chrono::high_resolution_clock::now();

	// Primary game loop.
	bool running = true;
	while (running)
	{
		const auto lastTime = thisTime;
		thisTime = std::chrono::high_resolution_clock::now();

		// Fastest allowed frame time in microseconds.
		const std::chrono::duration<int64_t, std::micro> minimumMS(
			1000000 / this->options->getTargetFPS());

		// Delay the current frame if the previous one was too fast.
		auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(thisTime - lastTime);
		if (frameTime < minimumMS)
		{
			const auto sleepTime = minimumMS - frameTime;
			std::this_thread::sleep_for(sleepTime);
			thisTime = std::chrono::high_resolution_clock::now();
			frameTime = std::chrono::duration_cast<std::chrono::microseconds>(thisTime - lastTime);
		}

		// Clamp the delta time to at most the maximum frame time.
		const double dt = std::fmin(frameTime.count(), maximumMS.count()) / 1000000.0;

		// Update the input manager's state.
		this->inputManager.update();

		// Update the audio manager, checking for finished sounds.
		this->audioManager.update();

		// Update FPS counter.
		this->fpsCounter.updateFrameTime(dt);

		// Listen for input events.
		this->handleEvents(running);

		// Animate the current game state by delta time.
		this->tick(dt);

		// Draw to the screen.
		this->render();
	}

	// At this point, the program has received an exit signal, and is now 
	// quitting peacefully.
	this->options->saveChanges();
}
