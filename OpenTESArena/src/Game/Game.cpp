#include <chrono>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "SDL.h"

#include "Game.h"
#include "Options.h"
#include "PlayerInterface.h"
#include "../Assets/CityDataFile.h"
#include "../Interface/Panel.h"
#include "../Interface/Surface.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/File.h"
#include "components/utilities/String.h"
#include "components/utilities/TextLinesFile.h"
#include "components/vfs/manager.hpp"

namespace
{
	// Size of scratch buffer in bytes, reset each frame.
	constexpr int SCRATCH_BUFFER_SIZE = 65536;
}

Game::Game()
{
	DebugLog("Initializing (Platform: " + Platform::getPlatform() + ").");

	// Get the current working directory. This is most relevant for platforms
	// like macOS, where the base path might be in the app's own "Resources" folder.
	this->basePath = Platform::getBasePath();

	// Get the path to the options folder. This is platform-dependent and points inside 
	// the "preferences directory" so it's always writable.
	this->optionsPath = Platform::getOptionsPath();

	// Parse options-default.txt and options-changes.txt (if it exists). Always prefer the
	// default file before the "changes" file.
	this->initOptions(this->basePath, this->optionsPath);

	// Initialize virtual file system using the Arena path in the options file.
	const bool arenaPathIsRelative = File::pathIsRelative(this->options.getMisc_ArenaPath().c_str());
	VFS::Manager::get().initialize(std::string(
		(arenaPathIsRelative ? this->basePath : "") + this->options.getMisc_ArenaPath()));

	// Initialize the OpenAL Soft audio manager.
	const bool midiPathIsRelative = File::pathIsRelative(this->options.getAudio_MidiConfig().c_str());
	const std::string midiPath = (midiPathIsRelative ? this->basePath : "") +
		this->options.getAudio_MidiConfig();

	this->audioManager.init(this->options.getAudio_MusicVolume(),
		this->options.getAudio_SoundVolume(), this->options.getAudio_SoundChannels(),
		this->options.getAudio_SoundResampling(), this->options.getAudio_Is3DAudio(), midiPath);

	// Initialize music library from file.
	const std::string musicLibraryPath = this->basePath + "data/audio/MusicDefinitions.txt";
	if (!this->musicLibrary.init(musicLibraryPath.c_str()))
	{
		DebugLogError("Couldn't init music library at \"" + musicLibraryPath + "\".");
	}

	// Initialize the renderer and window with the given settings.
	constexpr RendererSystemType2D rendererSystemType2D = RendererSystemType2D::SDL2;
	constexpr RendererSystemType3D rendererSystemType3D = RendererSystemType3D::SoftwareClassic;
	if (!this->renderer.init(this->options.getGraphics_ScreenWidth(), this->options.getGraphics_ScreenHeight(),
		static_cast<Renderer::WindowMode>(this->options.getGraphics_WindowMode()),
		this->options.getGraphics_LetterboxMode(), rendererSystemType2D, rendererSystemType3D))
	{
		throw DebugException("Couldn't init renderer.");
	}

	// Determine which version of the game the Arena path is pointing to.
	const bool isFloppyVersion = [this, arenaPathIsRelative]()
	{
		// Path to the Arena folder.
		const std::string fullArenaPath = [this, arenaPathIsRelative]()
		{
			// Include the base path if the ArenaPath is relative.
			const std::string path = (arenaPathIsRelative ? this->basePath : "") +
				this->options.getMisc_ArenaPath();
			return String::addTrailingSlashIfMissing(path);
		}();

		// Check for the CD version first.
		const std::string &acdExeName = ExeData::CD_VERSION_EXE_FILENAME;
		const std::string acdExePath = fullArenaPath + acdExeName;
		if (File::exists(acdExePath.c_str()))
		{
			DebugLog("CD version.");
			return false;
		}

		// If that's not there, check for the floppy disk version.
		const std::string &aExeName = ExeData::FLOPPY_VERSION_EXE_FILENAME;
		const std::string aExePath = fullArenaPath + aExeName;
		if (File::exists(aExePath.c_str()))
		{
			DebugLog("Floppy disk version.");
			return true;
		}

		// If neither exist, it's not a valid Arena directory.
		throw DebugException("\"" + fullArenaPath + "\" does not have an Arena executable.");
	}();

	// Load fonts.
	if (!this->fontLibrary.init())
	{
		DebugCrash("Couldn't init font library.");
	}

	// Load various asset libraries.
	if (!this->binaryAssetLibrary.init(isFloppyVersion))
	{
		DebugCrash("Couldn't init binary asset library.");
	}

	if (!this->textAssetLibrary.init())
	{
		DebugCrash("Couldn't init text asset library.");
	}

	// Load character classes (dependent on original game's data).
	this->charClassLibrary.init(this->binaryAssetLibrary.getExeData());

	this->cinematicLibrary.init();
	this->doorSoundLibrary.init();

	// Load entity definitions (dependent on original game's data).
	this->entityDefLibrary.init(this->binaryAssetLibrary.getExeData(), this->textureManager);

	// Load and set window icon.
	const Surface icon = [this]()
	{
		const std::string iconPath = this->basePath + "data/icon.bmp";
		Surface surface = Surface::loadBMP(iconPath.c_str(), Renderer::DEFAULT_PIXELFORMAT);

		// Treat black as transparent.
		const uint32_t black = surface.mapRGBA(0, 0, 0, 255);
		SDL_SetColorKey(surface.get(), SDL_TRUE, black);

		return surface;
	}();

	// Load single-instance sounds file for the audio manager.
	TextLinesFile singleInstanceSoundsFile;
	const std::string singleInstanceSoundsPath = this->basePath + "data/audio/SingleInstanceSounds.txt";
	if (singleInstanceSoundsFile.init(singleInstanceSoundsPath.c_str()))
	{
		for (int i = 0; i < singleInstanceSoundsFile.getLineCount(); i++)
		{
			const std::string &soundFilename = singleInstanceSoundsFile.getLine(i);
			this->audioManager.addSingleInstanceSound(std::string(soundFilename));
		}
	}
	else
	{
		DebugLogWarning("Missing single instance sounds file at \"" + singleInstanceSoundsPath + "\".");
	}

	this->renderer.setWindowIcon(icon);

	this->random.init();
	this->scratchAllocator.init(SCRATCH_BUFFER_SIZE);

	// Initialize panel and music to default.
	this->panel = Panel::defaultPanel(*this);
	
	const MusicDefinition *mainMenuMusicDef = this->musicLibrary.getRandomMusicDefinition(
		MusicDefinition::Type::MainMenu, this->random);
	if (mainMenuMusicDef == nullptr)
	{
		DebugLogWarning("Missing main menu music.");
	}

	this->audioManager.setMusic(mainMenuMusicDef);

	// Use a texture as the cursor instead.
	SDL_ShowCursor(SDL_FALSE);

	// Leave some members null for now. The game data is initialized when the player 
	// enters the game world, and the "next panel" is a temporary used by the game
	// to avoid corruption between panel events which change the panel.
	this->gameData = nullptr;
	this->charCreationState = nullptr;
	this->nextPanel = nullptr;
	this->nextSubPanel = nullptr;

	// This keeps the programmer from deleting a sub-panel the same frame it's in use.
	// The pop is delayed until the beginning of the next frame.
	this->requestedSubPanelPop = false;
}

Panel *Game::getActivePanel() const
{
	return (this->subPanels.size() > 0) ?
		this->subPanels.back().get() : this->panel.get();
}

AudioManager &Game::getAudioManager()
{
	return this->audioManager;
}

const MusicLibrary &Game::getMusicLibrary() const
{
	return this->musicLibrary;
}

InputManager &Game::getInputManager()
{
	return this->inputManager;
}

FontLibrary &Game::getFontLibrary()
{
	return this->fontLibrary;
}

const CinematicLibrary &Game::getCinematicLibrary() const
{
	return this->cinematicLibrary;
}

const CharacterClassLibrary &Game::getCharacterClassLibrary() const
{
	return this->charClassLibrary;
}

const DoorSoundLibrary &Game::getDoorSoundLibrary() const
{
	return this->doorSoundLibrary;
}

const EntityDefinitionLibrary &Game::getEntityDefinitionLibrary() const
{
	return this->entityDefLibrary;
}

bool Game::gameDataIsActive() const
{
	return this->gameData.get() != nullptr;
}

GameData &Game::getGameData() const
{
	DebugAssert(this->gameDataIsActive());
	return *this->gameData.get();
}

bool Game::characterCreationIsActive() const
{
	return this->charCreationState.get() != nullptr;
}

CharacterCreationState &Game::getCharacterCreationState() const
{
	DebugAssert(this->characterCreationIsActive());
	return *this->charCreationState.get();
}

Options &Game::getOptions()
{
	return this->options;
}

Renderer &Game::getRenderer()
{
	return this->renderer;
}

TextureManager &Game::getTextureManager()
{
	return this->textureManager;
}

const BinaryAssetLibrary &Game::getBinaryAssetLibrary() const
{
	return this->binaryAssetLibrary;
}

const TextAssetLibrary &Game::getTextAssetLibrary() const
{
	return this->textAssetLibrary;
}

Random &Game::getRandom()
{
	return this->random;
}

ScratchAllocator &Game::getScratchAllocator()
{
	return this->scratchAllocator;
}

Profiler &Game::getProfiler()
{
	return this->profiler;
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
	DebugAssertMsg(!this->requestedSubPanelPop, "Already scheduled to pop sub-panel.");

	// If there are no sub-panels, then there is only the main panel, and panels 
	// should never have any sub-panels to pop.
	DebugAssertMsg(this->subPanels.size() > 0, "No sub-panels to pop.");

	this->requestedSubPanelPop = true;
}

void Game::setGameData(std::unique_ptr<GameData> gameData)
{
	this->gameData = std::move(gameData);
}

void Game::setCharacterCreationState(std::unique_ptr<CharacterCreationState> charCreationState)
{
	this->charCreationState = std::move(charCreationState);
}

void Game::initOptions(const std::string &basePath, const std::string &optionsPath)
{
	// Load the default options first.
	const std::string defaultOptionsPath(basePath + "options/" + Options::DEFAULT_FILENAME);
	this->options.loadDefaults(defaultOptionsPath);

	// Check if the changes options file exists.
	const std::string changesOptionsPath(optionsPath + Options::CHANGES_FILENAME);
	if (!File::exists(changesOptionsPath.c_str()))
	{
		// Make one. Since the default options object has no changes, the new file will have
		// no key-value pairs.
		DebugLog("Creating options file at \"" + changesOptionsPath + "\".");
		this->options.saveChanges();
	}
	else
	{
		// Read in any key-value pairs in the "changes" options file.
		this->options.loadChanges(changesOptionsPath);
	}
}

void Game::resizeWindow(int width, int height)
{
	// Resize the window, and the 3D renderer if initialized.
	const bool fullGameWindow = this->options.getGraphics_ModernInterface();
	this->renderer.resize(width, height,
		this->options.getGraphics_ResolutionScale(), fullGameWindow);
}

void Game::saveScreenshot(const Surface &surface)
{
	// Get the path + filename to use for the new screenshot.
	const std::string screenshotPath = []()
	{
		const std::string screenshotFolder = Platform::getScreenshotPath();
		const std::string screenshotPrefix("screenshot");
		int imageIndex = 0;

		auto getNextAvailablePath = [&screenshotFolder, &screenshotPrefix, &imageIndex]()
		{
			std::stringstream ss;
			ss << std::setw(3) << std::setfill('0') << imageIndex;
			imageIndex++;
			return screenshotFolder + screenshotPrefix + ss.str() + ".bmp";
		};

		std::string path = getNextAvailablePath();
		while (File::exists(path.c_str()))
		{
			path = getNextAvailablePath();
		}

		return path;
	}();

	const int status = SDL_SaveBMP(surface.get(), screenshotPath.c_str());

	if (status == 0)
	{
		DebugLog("Screenshot saved to \"" + screenshotPath + "\".");
	}
	else
	{
		DebugCrash("Failed to save screenshot to \"" + screenshotPath + "\": " +
			std::string(SDL_GetError()));
	}
}

void Game::handlePanelChanges()
{
	// If a sub-panel pop was requested, then pop the top of the sub-panel stack.
	if (this->requestedSubPanelPop)
	{
		this->subPanels.pop_back();
		this->requestedSubPanelPop = false;
		
		// Unpause the panel that is now the top-most one.
		const bool paused = false;
		this->getActivePanel()->onPauseChanged(paused);
	}

	// If a new sub-panel was requested, then add it to the stack.
	if (this->nextSubPanel.get() != nullptr)
	{
		// Pause the top-most panel.
		const bool paused = true;
		this->getActivePanel()->onPauseChanged(paused);

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
			const auto &renderer = this->getRenderer();
			const Surface screenshot = renderer.getScreenshot();
			this->saveScreenshot(screenshot);
		}

		// Panel-specific events are handled by the active panel.
		this->getActivePanel()->handleEvent(e);

		// See if the event requested any changes in active panels.
		this->handlePanelChanges();
	}
}

void Game::tick(double dt)
{
	// Tick the active panel.
	this->getActivePanel()->tick(dt);

	// See if the panel tick requested any changes in active panels.
	this->handlePanelChanges();
}

void Game::render()
{
	// Draw the panel's main content.
	this->panel->render(this->renderer);

	// Draw any sub-panels back to front.
	for (auto &subPanel : this->subPanels)
	{
		subPanel->render(this->renderer);
	}

	// Call the active panel's secondary render method. Secondary render items are those
	// that are hidden on panels below the active one.
	Panel *activePanel = this->getActivePanel();
	activePanel->renderSecondary(this->renderer);

	// Get the active panel's cursor texture and alignment.
	const std::optional<Panel::CursorData> cursor = activePanel->getCurrentCursor();

	// Draw cursor if valid. Some panels do not define a cursor (like cinematics), so their cursor is empty.
	if (cursor.has_value())
	{
		// The panel should not be drawing the cursor themselves. It's done here 
		// just to make sure that the cursor is drawn only once and is always drawn last.
		this->renderer.drawCursor(cursor->getTextureBuilderID(), cursor->getPaletteID(), cursor->getAlignment(),
			this->inputManager.getMousePosition(), this->options.getGraphics_CursorScale(), this->textureManager);
	}

	this->renderer.present();
}

void Game::loop()
{
	// Nanoseconds per second. Only using this much precision because it's what
	// high_resolution_clock gives back. Microseconds would be fine too.
	constexpr int64_t timeUnits = 1000000000;

	// Longest allowed frame time.
	const std::chrono::duration<int64_t, std::nano> maxFrameTime(timeUnits / Options::MIN_FPS);

	// On some platforms, thread sleeping takes longer than it should, so include a value to
	// help compensate.
	std::chrono::nanoseconds sleepBias(0);

	auto thisTime = std::chrono::high_resolution_clock::now();

	// Primary game loop.
	bool running = true;
	while (running)
	{
		const auto lastTime = thisTime;
		thisTime = std::chrono::high_resolution_clock::now();

		// Shortest allowed frame time.
		const std::chrono::duration<int64_t, std::nano> minFrameTime(
			timeUnits / this->options.getGraphics_TargetFPS());

		// Time since the last frame started.
		const auto frameTime = [minFrameTime, &sleepBias, &thisTime, lastTime]()
		{
			// Delay the current frame if the previous one was too fast.
			auto diff = thisTime - lastTime;
			if (diff < minFrameTime)
			{
				const auto sleepTime = minFrameTime - diff + sleepBias;
				std::this_thread::sleep_for(sleepTime);

				// Compensate for sleeping too long. Thread sleeping has questionable accuracy.
				const auto tempTime = std::chrono::high_resolution_clock::now();
				const auto unnecessarySleepTime = [thisTime, sleepTime, tempTime]()
				{
					const auto tempFrameTime = tempTime - thisTime;
					return tempFrameTime - sleepTime;
				}();

				sleepBias = -unnecessarySleepTime;
				thisTime = tempTime;
				diff = thisTime - lastTime;
			}

			return diff;
		}();

		// Two delta times: actual and clamped. Use the clamped delta time for game calculations
		// so things don't break at low frame rates.
		constexpr double timeUnitsReal = static_cast<double>(timeUnits);
		const double dt = static_cast<double>(frameTime.count()) / timeUnitsReal;
		const double clampedDt = std::fmin(frameTime.count(), maxFrameTime.count()) / timeUnitsReal;

		// Reset scratch allocator for use with this frame.
		this->scratchAllocator.clear();

		// Update the input manager's state.
		this->inputManager.update();

		// Update the audio manager listener (if any) and check for finished sounds.
		if (this->gameDataIsActive())
		{
			const AudioManager::ListenerData listenerData = [this]()
			{
				const Player &player = this->getGameData().getPlayer();
				const NewDouble3 absolutePosition = VoxelUtils::coordToNewPoint(player.getPosition());
				const NewDouble3 &direction = player.getDirection();
				return AudioManager::ListenerData(absolutePosition, direction);
			}();

			this->audioManager.update(dt, &listenerData);
		}
		else
		{
			this->audioManager.update(dt, nullptr);
		}

		// Update FPS counter.
		this->fpsCounter.updateFrameTime(dt);

		// Listen for input events.
		try
		{
			this->handleEvents(running);
		}
		catch (const std::exception &e)
		{
			DebugCrash("handleEvents() exception! " + std::string(e.what()));
		}

		// Animate the current game state by delta time.
		try
		{
			// Multiply delta time by the time scale. I settled on having the effects of this
			// be application-wide rather than just in the game world since it's intended to
			// simulate lower DOSBox cycles.
			const double timeScaledDt = clampedDt * this->options.getMisc_TimeScale();
			this->tick(timeScaledDt);
		}
		catch (const std::exception &e)
		{
			DebugCrash("tick() exception! " + std::string(e.what()));
		}

		// Draw to the screen.
		try
		{
			this->render();
		}
		catch (const std::exception &e)
		{
			DebugCrash("render() exception! " + std::string(e.what()));
		}
	}

	// At this point, the program has received an exit signal, and is now 
	// quitting peacefully.
	this->options.saveChanges();
}
