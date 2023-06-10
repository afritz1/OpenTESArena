#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "SDL.h"

#include "Game.h"
#include "Options.h"
#include "PlayerInterface.h"
#include "../Assets/ArenaLevelLibrary.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Assets/TextureManager.h"
#include "../Audio/MusicLibrary.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../GameLogic/PlayerLogicController.h"
#include "../Input/InputActionName.h"
#include "../Interface/CinematicLibrary.h"
#include "../Interface/CommonUiController.h"
#include "../Interface/CommonUiView.h"
#include "../Interface/GameWorldPanel.h"
#include "../Interface/GameWorldUiModel.h"
#include "../Interface/IntroUiModel.h"
#include "../Interface/Panel.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/GuiUtils.h"
#include "../UI/Surface.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/Directory.h"
#include "components/utilities/File.h"
#include "components/utilities/Path.h"
#include "components/utilities/String.h"
#include "components/utilities/TextLinesFile.h"
#include "components/vfs/manager.hpp"

namespace
{
	bool TryMakeValidArenaExePath(const std::string &vfsFolderPath, std::string *outExePath, bool *outIsFloppyDiskVersion)
	{
		// Check for CD version first.
		const std::string &cdExeName = ExeData::CD_VERSION_EXE_FILENAME;
		std::string cdExePath = vfsFolderPath + cdExeName;
		if (File::exists(cdExePath.c_str()))
		{
			DebugLog("CD executable \"" + cdExeName + "\" found in \"" + vfsFolderPath + "\".");
			*outExePath = std::move(cdExePath);
			*outIsFloppyDiskVersion = false;
			return true;
		}

		const std::string &floppyDiskExeName = ExeData::FLOPPY_VERSION_EXE_FILENAME;
		std::string floppyDiskExePath = vfsFolderPath + floppyDiskExeName;
		if (File::exists(floppyDiskExePath.c_str()))
		{
			DebugLog("Floppy disk executable \"" + floppyDiskExeName + "\" found in \"" + vfsFolderPath + "\".");
			*outExePath = std::move(floppyDiskExePath);
			*outIsFloppyDiskVersion = true;
			return true;
		}

		// No valid Arena .exe found.
		return false;
	}
}

Game::Game()
{
	// Keeps us from deleting a sub-panel the same frame it's in use. The pop is delayed until the
	// beginning of the next frame.
	this->requestedSubPanelPop = false;

	this->shouldSimulateScene = false;
	this->running = true;
}

Game::~Game()
{
	if (this->applicationExitListenerID.has_value())
	{
		this->inputManager.removeListener(*this->applicationExitListenerID);
	}

	if (this->windowResizedListenerID.has_value())
	{
		this->inputManager.removeListener(*this->windowResizedListenerID);
	}

	if (this->takeScreenshotListenerID.has_value())
	{
		this->inputManager.removeListener(*this->takeScreenshotListenerID);
	}

	if (this->debugProfilerListenerID.has_value())
	{
		this->inputManager.removeListener(*this->debugProfilerListenerID);
	}

	RenderChunkManager &renderChunkManager = this->sceneManager.renderChunkManager;
	renderChunkManager.shutdown(this->renderer);
}

bool Game::init()
{
	DebugLog("Initializing (Platform: " + Platform::getPlatform() + ").");

	// Current working directory (in most cases). This is most relevant for platforms like macOS, where
	// the base path might be in the app's Resources folder.
	const std::string basePath = Platform::getBasePath();
	const std::string dataFolderPath = basePath + "data/";

	// Initialize options from default and changes files if present. The path is platform-dependent
	// and points inside the preferences directory so it's always writable.
	const std::string optionsPath = Platform::getOptionsPath();
	this->initOptions(basePath, optionsPath);

	const std::string &arenaPath = this->options.getMisc_ArenaPath();
	DebugLog("Using ArenaPath \"" + arenaPath + "\".");

	// Initialize virtual file system using the Arena path in the options file.
	const bool arenaPathIsRelative = Path::isRelative(arenaPath.c_str());
	const std::string vfsFolderPath = String::addTrailingSlashIfMissing((arenaPathIsRelative ? basePath : "") + arenaPath);
	if (!Directory::exists(vfsFolderPath.c_str()))
	{
		DebugLogError("Data files directory \"" + vfsFolderPath + "\" not found. Is your ArenaPath correct?");
		return false;
	}

	VFS::Manager::get().initialize(std::string(vfsFolderPath));

	// Determine which game version the data path is pointing to.
	std::string arenaExePath;
	bool isFloppyDiskVersion;
	if (!TryMakeValidArenaExePath(vfsFolderPath, &arenaExePath, &isFloppyDiskVersion))
	{
		DebugLogError("\"" + vfsFolderPath + "\" does not contain an Arena executable.");
		return false;
	}

	// Initialize audio manager.
	const bool midiPathIsRelative = Path::isRelative(this->options.getAudio_MidiConfig().c_str());
	const std::string midiFilePath = (midiPathIsRelative ? basePath : "") + this->options.getAudio_MidiConfig();
	const std::string audioDataPath = dataFolderPath + "audio/";
	this->audioManager.init(this->options.getAudio_MusicVolume(), this->options.getAudio_SoundVolume(),
		this->options.getAudio_SoundChannels(), this->options.getAudio_SoundResampling(),
		this->options.getAudio_Is3DAudio(), midiFilePath, audioDataPath);

	// Initialize the renderer and window with the given settings.
	auto resolutionScaleFunc = [this]()
	{
		const auto &options = this->getOptions();
		return options.getGraphics_ResolutionScale();
	};

	constexpr RendererSystemType2D rendererSystemType2D = RendererSystemType2D::SDL2;
	constexpr RendererSystemType3D rendererSystemType3D = RendererSystemType3D::SoftwareClassic;
	if (!this->renderer.init(this->options.getGraphics_ScreenWidth(), this->options.getGraphics_ScreenHeight(),
		static_cast<Renderer::WindowMode>(this->options.getGraphics_WindowMode()),
		this->options.getGraphics_LetterboxMode(), this->options.getGraphics_ModernInterface(),
		resolutionScaleFunc, rendererSystemType2D, rendererSystemType3D, this->options.getGraphics_RenderThreadsMode()))
	{
		DebugLogError("Couldn't init renderer (2D: " + std::to_string(static_cast<int>(rendererSystemType2D)) +
			", 3D: " + std::to_string(static_cast<int>(rendererSystemType3D)) + ").");
		return false;
	}

	RenderChunkManager &renderChunkManager = this->sceneManager.renderChunkManager;
	renderChunkManager.init(this->renderer);

	this->inputManager.init();

	// Add application-level input event handlers.
	this->applicationExitListenerID = this->inputManager.addApplicationExitListener(
		[this]()
	{
		this->handleApplicationExit();
	});

	this->windowResizedListenerID = this->inputManager.addWindowResizedListener(
		[this](int width, int height)
	{
		this->handleWindowResized(width, height);
	});

	this->takeScreenshotListenerID = this->inputManager.addInputActionListener(InputActionName::Screenshot,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			// Save a screenshot to the local folder.
			const auto &renderer = this->getRenderer();
			const Surface screenshot = renderer.getScreenshot();
			this->saveScreenshot(screenshot);
		}
	});

	this->debugProfilerListenerID = this->inputManager.addInputActionListener(
		InputActionName::DebugProfiler, CommonUiController::onDebugInputAction);

	// Load various asset libraries.
	if (!FontLibrary::getInstance().init())
	{
		DebugLogError("Couldn't init font library.");
		return false;
	}

	if (!ArenaLevelLibrary::getInstance().init())
	{
		DebugLogError("Couldn't init Arena level library.");
		return false;
	}

	BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	if (!binaryAssetLibrary.init(isFloppyDiskVersion))
	{
		DebugLogError("Couldn't init binary asset library.");
		return false;
	}

	if (!TextAssetLibrary::getInstance().init())
	{
		DebugLogError("Couldn't init text asset library.");
		return false;
	}

	const std::string musicLibraryPath = audioDataPath + "MusicDefinitions.txt";
	if (!MusicLibrary::getInstance().init(musicLibraryPath.c_str()))
	{
		DebugLogError("Couldn't init music library with path \"" + musicLibraryPath + "\".");
		return false;
	}

	CinematicLibrary::getInstance().init();

	const ExeData &exeData = binaryAssetLibrary.getExeData();
	CharacterClassLibrary::getInstance().init(exeData);
	EntityDefinitionLibrary::getInstance().init(exeData, this->textureManager);

	// Initialize window icon.
	const std::string windowIconPath = dataFolderPath + "icon.bmp";
	const Surface windowIconSurface = Surface::loadBMP(windowIconPath.c_str(), Renderer::DEFAULT_PIXELFORMAT);
	if (windowIconSurface.get() == nullptr)
	{
		DebugLogError("Couldn't load window icon with path \"" + windowIconPath + "\".");
		return false;
	}

	const uint32_t windowIconColorKey = windowIconSurface.mapRGBA(0, 0, 0, 255);
	SDL_SetColorKey(windowIconSurface.get(), SDL_TRUE, windowIconColorKey);
	this->renderer.setWindowIcon(windowIconSurface);

	// Initialize click regions for player movement in classic interface mode.
	const Int2 windowDims = this->renderer.getWindowDimensions();
	this->updateNativeCursorRegions(windowDims.x, windowDims.y);

	// Random seed.
	this->random.init();

	// Use an in-game texture as the cursor instead of system cursor.
	SDL_ShowCursor(SDL_FALSE);

	// Leave some members null for now. The "next panel" is a temporary used by the game
	// to avoid corruption between panel events which change the panel.
	DebugAssert(this->charCreationState == nullptr);
	DebugAssert(this->nextPanel == nullptr);
	DebugAssert(this->nextSubPanel == nullptr);

	return true;
}

Panel *Game::getActivePanel() const
{
	return (this->subPanels.size() > 0) ? this->subPanels.back().get() : this->panel.get();
}

AudioManager &Game::getAudioManager()
{
	return this->audioManager;
}

InputManager &Game::getInputManager()
{
	return this->inputManager;
}

GameState &Game::getGameState()
{
	return this->gameState;
}

Player &Game::getPlayer()
{
	return this->player;
}

SceneManager &Game::getSceneManager()
{
	return this->sceneManager;
}

bool Game::isSimulatingScene() const
{
	return this->shouldSimulateScene;
}

void Game::setIsSimulatingScene(bool active)
{
	this->shouldSimulateScene = active;
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

Random &Game::getRandom()
{
	return this->random;
}

ArenaRandom &Game::getArenaRandom()
{
	return this->arenaRandom;
}

Profiler &Game::getProfiler()
{
	return this->profiler;
}

const FPSCounter &Game::getFPSCounter() const
{
	return this->fpsCounter;
}

const Rect &Game::getNativeCursorRegion(int index) const
{
	DebugAssertIndex(this->nativeCursorRegions, index);
	return this->nativeCursorRegions[index];
}

TextBox *Game::getTriggerTextBox()
{
	Panel *panel = this->getActivePanel();
	if (panel == nullptr)
	{
		DebugLogError("No active panel for trigger text box getter.");
		return nullptr;
	}

	GameWorldPanel *gameWorldPanel = dynamic_cast<GameWorldPanel*>(panel);
	if (gameWorldPanel == nullptr)
	{
		DebugLogError("Active panel is not game world panel for trigger text box getter.");
		return nullptr;
	}

	return &gameWorldPanel->getTriggerTextBox();
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

void Game::setCharacterCreationState(std::unique_ptr<CharacterCreationState> charCreationState)
{
	this->charCreationState = std::move(charCreationState);
}

void Game::setGameWorldRenderCallback(const GameWorldRenderCallback &callback)
{
	this->gameWorldRenderCallback = callback;
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

void Game::resizeWindow(int windowWidth, int windowHeight)
{
	// Resize the window, and the 3D renderer if initialized.
	const bool fullGameWindow = this->options.getGraphics_ModernInterface();
	this->renderer.resize(windowWidth, windowHeight,
		this->options.getGraphics_ResolutionScale(), fullGameWindow);

	// Update where the mouse can click for player movement in the classic interface.
	this->updateNativeCursorRegions(windowWidth, windowHeight);
}

void Game::saveScreenshot(const Surface &surface)
{
	const std::string directoryName = Platform::getScreenshotPath();
	const char *directoryNamePtr = directoryName.c_str();
	if (!Directory::exists(directoryNamePtr))
	{
		Directory::createRecursively(directoryNamePtr);
	}	

	std::error_code code;
	const std::filesystem::directory_iterator dirIter(directoryName, code);
	if (code)
	{
		DebugLogWarning("Couldn't create directory iterator for \"" + std::string(directoryName) + "\": " + code.message());
		return;
	}
	
	const std::string prefix("screenshot");
	const std::string suffix(".bmp");
	constexpr size_t expectedNumberDigits = 4; // 0-9999; if it reaches 10000 then that one gets overwritten.

	int maxFoundNumber = -1;
	for (const std::filesystem::directory_entry &entry : dirIter)
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const std::string entryFilename = entry.path().filename().string();
		const size_t prefixIndex = 0;
		const size_t numberStartIndex = prefixIndex + prefix.size();
		const size_t suffixIndex = entryFilename.find(suffix, numberStartIndex);
		if (suffixIndex == std::string::npos)
		{
			continue;
		}

		const size_t numberEndIndex = suffixIndex;
		const std::string numberStr = entryFilename.substr(numberStartIndex, numberEndIndex - numberStartIndex);
		if (numberStr.size() != expectedNumberDigits)
		{
			continue;
		}

		const int number = std::stoi(numberStr.c_str());
		if (number > maxFoundNumber)
		{
			maxFoundNumber = number;
		}
	}

	const int actualNumber = maxFoundNumber + 1;
	std::stringstream ss;
	ss << std::setw(expectedNumberDigits) << std::setfill('0') << actualNumber;
	const std::string screenshotPath = directoryName + prefix + ss.str() + suffix;
	const int status = SDL_SaveBMP(surface.get(), screenshotPath.c_str());
	if (status == 0)
	{
		DebugLog("Screenshot saved to \"" + screenshotPath + "\".");
	}
	else
	{
		DebugLogError("Failed to save screenshot to \"" + screenshotPath + "\": " + std::string(SDL_GetError()));
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
		this->getActivePanel()->onPauseChanged(false);
	}

	// If a new panel was requested, switch to it.
	if (this->nextPanel.get() != nullptr)
	{
		this->panel = std::move(this->nextPanel);
	}

	// If a new sub-panel was requested, then add it to the stack.
	if (this->nextSubPanel.get() != nullptr)
	{
		// Pause the top-most panel.
		this->getActivePanel()->onPauseChanged(true);

		this->subPanels.emplace_back(std::move(this->nextSubPanel));
	}
}

void Game::handleApplicationExit()
{
	this->running = false;
}

void Game::handleWindowResized(int width, int height)
{
	this->resizeWindow(width, height);

	// Call each panel's resize method. The panels should not be listening for resize events themselves
	// because it's more of an application event than a panel event.
	this->panel->resize(width, height);

	for (auto &subPanel : this->subPanels)
	{
		subPanel->resize(width, height);
	}
}

void Game::updateNativeCursorRegions(int windowWidth, int windowHeight)
{
	// Update screen regions for classic interface player movement.
	GameWorldUiModel::updateNativeCursorRegions(BufferView<Rect>(this->nativeCursorRegions), windowWidth, windowHeight);
}

void Game::renderDebugInfo()
{
	const int profilerLevel = this->options.getMisc_ProfilerLevel();
	if (profilerLevel == Options::MIN_PROFILER_LEVEL)
	{
		return;
	}

	std::string debugText;
	if (profilerLevel >= 1)
	{
		// FPS.
		const double averageFps = this->fpsCounter.getAverageFPS();
		const double highestFps = this->fpsCounter.getHighestFPS();
		const double lowestFps = this->fpsCounter.getLowestFPS();
		const double averageFrameTimeMS = 1000.0 / averageFps;
		const double lowestFrameTimeMS = 1000.0 / highestFps;
		const double highestFrameTimeMS = 1000.0 / lowestFps;
		const std::string averageFpsText = String::fixedPrecision(averageFps, 0);
		const std::string averageFrameTimeText = String::fixedPrecision(averageFrameTimeMS, 1);
		const std::string lowestFrameTimeText = String::fixedPrecision(lowestFrameTimeMS, 1);
		const std::string highestFrameTimeText = String::fixedPrecision(highestFrameTimeMS, 1);
		debugText.append("FPS: " + averageFpsText + " (" + averageFrameTimeText + "ms " + lowestFrameTimeText +
			"ms " + highestFrameTimeText + "ms)");
	}

	const Int2 windowDims = this->renderer.getWindowDimensions();
	if (profilerLevel >= 2)
	{
		// Renderer details (window res, render res, threads, frame times, etc.).
		const std::string windowWidth = std::to_string(windowDims.x);
		const std::string windowHeight = std::to_string(windowDims.y);
		debugText.append("\nScreen: " + windowWidth + "x" + windowHeight);

		const Renderer::ProfilerData &profilerData = this->renderer.getProfilerData();
		const Int2 renderDims(profilerData.width, profilerData.height);
		const bool profilerDataIsValid = (renderDims.x > 0) && (renderDims.y > 0);
		if (profilerDataIsValid)
		{
			const double resolutionScale = this->options.getGraphics_ResolutionScale();
			const std::string renderWidth = std::to_string(renderDims.x);
			const std::string renderHeight = std::to_string(renderDims.y);
			const std::string renderResScale = String::fixedPrecision(resolutionScale, 2);
			const std::string renderThreadCount = std::to_string(profilerData.threadCount);
			const std::string renderTime = String::fixedPrecision(profilerData.frameTime * 1000.0, 2);
			const std::string renderDrawCallCount = std::to_string(profilerData.drawCallCount);
			debugText.append("\nRender: " + renderWidth + "x" + renderHeight + " (" + renderResScale + "), " +
				renderThreadCount + " thread" + ((profilerData.threadCount > 1) ? "s" : "") + '\n' +
				"3D render: " + renderTime + "ms" + "\n" +
				"Draw calls: " + renderDrawCallCount + "\n" +
				"Vis triangles: " + std::to_string(profilerData.visTriangleCount) + " (" + std::to_string(profilerData.potentiallyVisTriangleCount) + ")" +
				", lights: " + std::to_string(profilerData.visLightCount));
		}
		else
		{
			debugText.append("\nNo profiler data available.");
		}
	}

	if (profilerLevel >= 3)
	{
		// Player position, direction, etc.
		const CoordDouble3 &playerPosition = this->player.getPosition();
		const Double3 &direction = this->player.getDirection();

		const std::string chunkStr = playerPosition.chunk.toString();
		const std::string chunkPosX = String::fixedPrecision(playerPosition.point.x, 2);
		const std::string chunkPosY = String::fixedPrecision(playerPosition.point.y, 2);
		const std::string chunkPosZ = String::fixedPrecision(playerPosition.point.z, 2);
		const std::string dirX = String::fixedPrecision(direction.x, 2);
		const std::string dirY = String::fixedPrecision(direction.y, 2);
		const std::string dirZ = String::fixedPrecision(direction.z, 2);

		debugText.append("\nChunk: " + chunkStr + '\n' +
			"Chunk pos: " + chunkPosX + ", " + chunkPosY + ", " + chunkPosZ + '\n' +
			"Dir: " + dirX + ", " + dirY + ", " + dirZ);
	}

	this->debugInfoTextBox.setText(debugText);

	const UiTextureID textureID = this->debugInfoTextBox.getTextureID();
	const Rect &debugInfoRect = this->debugInfoTextBox.getRect();
	const Int2 position = debugInfoRect.getTopLeft();
	const Int2 size(debugInfoRect.getWidth(), debugInfoRect.getHeight());
	constexpr PivotType pivotType = PivotType::TopLeft;
	constexpr RenderSpace renderSpace = RenderSpace::Classic;

	double xPercent, yPercent, wPercent, hPercent;
	GuiUtils::makeRenderElementPercents(position.x, position.y, size.x, size.y, windowDims.x, windowDims.y,
		renderSpace, pivotType, &xPercent, &yPercent, &wPercent, &hPercent);

	const RendererSystem2D::RenderElement renderElement(textureID, xPercent, yPercent, wPercent, hPercent);
	this->renderer.draw(&renderElement, 1, renderSpace);
}

void Game::loop()
{
	// Initialize panel and music to default (bootstrapping the first game frame).
	this->panel = IntroUiModel::makeStartupPanel(*this);

	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *mainMenuMusicDef = musicLibrary.getRandomMusicDefinition(MusicDefinition::Type::MainMenu, this->random);
	if (mainMenuMusicDef == nullptr)
	{
		DebugLogWarning("Missing main menu music.");
	}

	this->audioManager.setMusic(mainMenuMusicDef);

	const TextBox::InitInfo debugInfoTextBoxInitInfo = CommonUiView::getDebugInfoTextBoxInitInfo(FontLibrary::getInstance());
	if (!this->debugInfoTextBox.init(debugInfoTextBoxInitInfo, this->renderer))
	{
		DebugCrash("Couldn't init debug info text box.");
	}

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
	while (this->running)
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

		// Update FPS counter.
		this->fpsCounter.updateFrameTime(dt);

		// User input.
		try
		{
			const BufferView<const ButtonProxy> buttonProxies = this->getActivePanel()->getButtonProxies();
			auto onFinishedProcessingEventFunc = [this]()
			{
				// See if the event requested any changes in active panels.
				this->handlePanelChanges();
			};

			this->inputManager.update(*this, dt, buttonProxies, onFinishedProcessingEventFunc);

			if (this->isSimulatingScene())
			{
				// Handle input for player motion.
				const BufferView<const Rect> nativeCursorRegionsView(this->nativeCursorRegions);
				const Double2 playerTurnDeltaXY = PlayerLogicController::makeTurningAngularValues(*this, clampedDt, nativeCursorRegionsView);
				PlayerLogicController::turnPlayer(*this, playerTurnDeltaXY.x, playerTurnDeltaXY.y);
				PlayerLogicController::handlePlayerMovement(*this, clampedDt, nativeCursorRegionsView);
			}
		}
		catch (const std::exception &e)
		{
			DebugCrash("User input exception: " + std::string(e.what()));
		}

		// Tick.
		try
		{
			// Animate the current UI panel by delta time.
			this->getActivePanel()->tick(clampedDt);

			// See if the panel tick requested any changes in active panels.
			this->handlePanelChanges();

			if (this->isSimulatingScene() && this->gameState.isActiveMapValid())
			{
				// Recalculate the active chunks.
				const CoordDouble3 playerCoord = this->player.getPosition();
				const int chunkDistance = this->options.getMisc_ChunkDistance();
				ChunkManager &chunkManager = this->sceneManager.chunkManager;
				chunkManager.update(playerCoord.chunk, chunkDistance);

				// @todo: we should be able to get the voxel/entity/collision/etc. managers right here.
				// It shouldn't be abstracted into a game state.
				// - it should be like "do we need to clear the scene? yes/no. update the scene immediately? yes/no"

				// Tick game world state (daytime clock, etc.).
				this->gameState.tickGameClock(clampedDt, *this);
				this->gameState.tickChasmAnimation(clampedDt);
				this->gameState.tickWeather(clampedDt, *this);
				this->gameState.tickUiMessages(clampedDt);
				this->gameState.tickPlayer(clampedDt, *this);
				this->gameState.tickVoxels(clampedDt, *this);
				this->gameState.tickEntities(clampedDt, *this);
				this->gameState.tickCollision(clampedDt, *this);
				this->gameState.tickRendering(*this);

				// Update audio listener orientation.
				const WorldDouble3 absolutePosition = VoxelUtils::coordToWorldPoint(playerCoord);
				const WorldDouble3 &direction = this->player.getDirection();
				const AudioManager::ListenerData listenerData(absolutePosition, direction);
				this->audioManager.updateListener(listenerData);
			}

			this->audioManager.updateSources();
		}
		catch (const std::exception &e)
		{
			DebugCrash("Tick exception: " + std::string(e.what()));
		}

		// Late tick. User input, ticking the active panel, and simulating the game state all have the potential
		// to queue a scene change which needs to be fully processed before we render.
		try
		{
			this->gameState.tryUpdatePendingMapTransition(*this, clampedDt);
		}
		catch (const std::exception &e)
		{
			DebugCrash("Late tick exception: " + std::string(e.what()));
		}

		// Render.
		try
		{
			// Get the draw calls from each UI panel/sub-panel and determine what to draw.
			std::vector<const Panel*> panelsToRender;
			panelsToRender.emplace_back(this->panel.get());
			for (const auto &subPanel : this->subPanels)
			{
				panelsToRender.emplace_back(subPanel.get());
			}

			this->renderer.clear();

			if (this->gameWorldRenderCallback)
			{
				if (!this->gameWorldRenderCallback(*this))
				{
					DebugLogError("Couldn't render game world.");
				}
			}

			const Int2 windowDims = this->renderer.getWindowDimensions();

			for (const Panel *currentPanel : panelsToRender)
			{
				const BufferView<const UiDrawCall> drawCallsView = currentPanel->getDrawCalls();
				for (const UiDrawCall &drawCall : drawCallsView)
				{
					if (!drawCall.isActive())
					{
						continue;
					}

					const std::optional<Rect> &optClipRect = drawCall.getClipRect();
					if (optClipRect.has_value())
					{
						const SDL_Rect clipRect = optClipRect->getSdlRect();
						this->renderer.setClipRect(&clipRect);
					}

					const UiTextureID textureID = drawCall.getTextureID();
					const Int2 position = drawCall.getPosition();
					const Int2 size = drawCall.getSize();
					const PivotType pivotType = drawCall.getPivotType();
					const RenderSpace renderSpace = drawCall.getRenderSpace();

					double xPercent, yPercent, wPercent, hPercent;
					GuiUtils::makeRenderElementPercents(position.x, position.y, size.x, size.y, windowDims.x, windowDims.y,
						renderSpace, pivotType, &xPercent, &yPercent, &wPercent, &hPercent);

					const RendererSystem2D::RenderElement renderElement(textureID, xPercent, yPercent, wPercent, hPercent);
					this->renderer.draw(&renderElement, 1, renderSpace);

					if (optClipRect.has_value())
					{
						this->renderer.setClipRect(nullptr);
					}
				}
			}

			this->renderDebugInfo();
			this->renderer.present();
		}
		catch (const std::exception &e)
		{
			DebugCrash("Render exception: " + std::string(e.what()));
		}

		// End-of-frame clean up.
		try
		{
			this->sceneManager.cleanUp();
		}
		catch (const std::exception &e)
		{
			DebugCrash("Clean-up exception: " + std::string(e.what()));
		}
	}

	// At this point, the engine has received an exit signal and is now quitting peacefully.
	this->options.saveChanges();
}
