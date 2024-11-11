#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "Jolt/Jolt.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "SDL.h"

#include "ClockLibrary.h"
#include "Game.h"
#include "Options.h"
#include "PlayerInterface.h"
#include "../Assets/ArenaLevelLibrary.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Assets/TextureManager.h"
#include "../Audio/MusicLibrary.h"
#include "../Collision/Physics.h"
#include "../Collision/PhysicsBodyActivationListener.h"
#include "../Collision/PhysicsContactListener.h"
#include "../Collision/PhysicsLayer.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../GameLogic/PlayerLogicController.h"
#include "../Input/InputActionName.h"
#include "../Interface/CinematicLibrary.h"
#include "../Interface/CommonUiController.h"
#include "../Interface/CommonUiView.h"
#include "../Interface/GameWorldPanel.h"
#include "../Interface/GameWorldUiModel.h"
#include "../Interface/GameWorldUiView.h"
#include "../Interface/IntroUiModel.h"
#include "../Interface/Panel.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/GuiUtils.h"
#include "../UI/Surface.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/Directory.h"
#include "components/utilities/File.h"
#include "components/utilities/Path.h"
#include "components/utilities/Profiler.h"
#include "components/utilities/String.h"
#include "components/utilities/TextLinesFile.h"
#include "components/vfs/manager.hpp"

namespace
{
	struct FrameTimer
	{
		std::chrono::nanoseconds maximumTime; // Longest allowed frame time before engine will run in slow motion.
		std::chrono::nanoseconds minimumTime; // Shortest allowed frame time if not enough work is happening.
		std::chrono::time_point<std::chrono::high_resolution_clock> previousTime, currentTime;
		std::chrono::nanoseconds sleepBias; // Thread sleeping takes longer than it should on some platforms.
		double deltaTime; // Difference between frame times in seconds.
		double clampedDeltaTime; // For game logic calculations that become imprecise or break at low FPS.
		int physicsSteps; // 1 unless the engine has to do more steps this frame to keep numeric accuracy.

		FrameTimer()
		{
			this->maximumTime = std::chrono::nanoseconds(0);
			this->minimumTime = std::chrono::nanoseconds(0);
			this->sleepBias = std::chrono::nanoseconds(0);
			this->deltaTime = 0.0;
			this->clampedDeltaTime = 0.0;
			this->physicsSteps = 0;
		}

		void init()
		{
			this->maximumTime = std::chrono::nanoseconds(std::nano::den / Options::MIN_FPS);
			this->currentTime = std::chrono::high_resolution_clock::now();
			this->sleepBias = std::chrono::nanoseconds::zero();
		}

		void startFrame(int targetFPS)
		{
			DebugAssert(targetFPS > 0);
			this->minimumTime = std::chrono::nanoseconds(std::nano::den / targetFPS);
			this->previousTime = this->currentTime;
			this->currentTime = std::chrono::high_resolution_clock::now();

			auto previousFrameDuration = this->currentTime - this->previousTime;
			if (previousFrameDuration < this->minimumTime)
			{
				const auto sleepDuration = this->minimumTime - previousFrameDuration + this->sleepBias;
				std::this_thread::sleep_for(sleepDuration);

				const auto currentTimeAfterSleeping = std::chrono::high_resolution_clock::now();
				const auto sleptDuration = currentTimeAfterSleeping - this->currentTime;
				const auto oversleptDuration = sleptDuration - sleepDuration;

				this->sleepBias = -oversleptDuration;
				this->currentTime = currentTimeAfterSleeping;
				previousFrameDuration = this->currentTime - this->previousTime;
			}

			constexpr double timeUnitsReal = static_cast<double>(std::nano::den);
			this->deltaTime = static_cast<double>(previousFrameDuration.count()) / timeUnitsReal;
			this->clampedDeltaTime = std::fmin(previousFrameDuration.count(), this->maximumTime.count()) / timeUnitsReal;
			this->physicsSteps = static_cast<int>(std::ceil(this->clampedDeltaTime / Physics::DeltaTime));
		}
	};

	bool TryGetArenaAssetsDirectory(BufferView<const std::string> arenaPaths, const std::string &basePath, std::string *outDirectory, bool *outIsFloppyDiskVersion)
	{
		std::vector<std::string> validArenaPaths;
		for (std::string path : arenaPaths)
		{
			if (path.empty())
			{
				continue;
			}

			if (Path::isRelative(path.c_str()))
			{
				path = basePath + path;
			}

			validArenaPaths.emplace_back(std::move(path));
		}

		// Check for CD version first.
		for (const std::string &path : validArenaPaths)
		{
			const std::filesystem::path fsPath(path);
			std::error_code dummy;
			if (!std::filesystem::exists(fsPath, dummy) || !std::filesystem::is_directory(fsPath, dummy))
			{
				continue;
			}

			const std::string &cdExeName = ExeData::CD_VERSION_EXE_FILENAME;
			const std::filesystem::path cdExePath = fsPath / cdExeName;
			if (!std::filesystem::exists(cdExePath, dummy) || !std::filesystem::is_regular_file(cdExePath, dummy))
			{
				continue;
			}

			DebugLog("CD version assets found in \"" + path + "\".");
			*outDirectory = path;
			*outIsFloppyDiskVersion = false;
			return true;
		}

		for (const std::string &path : validArenaPaths)
		{
			const std::filesystem::path fsPath(path);
			std::error_code dummy;
			if (!std::filesystem::exists(fsPath, dummy) || !std::filesystem::is_directory(fsPath, dummy))
			{
				continue;
			}

			const std::string &floppyDiskExeName = ExeData::FLOPPY_VERSION_EXE_FILENAME;
			const std::filesystem::path floppyDiskExePath = fsPath / floppyDiskExeName;
			if (!std::filesystem::exists(floppyDiskExePath, dummy) || !std::filesystem::is_regular_file(floppyDiskExePath, dummy))
			{
				continue;
			}

			DebugLog("Floppy disk version assets found in \"" + path + "\".");
			*outDirectory = path;
			*outIsFloppyDiskVersion = true;
			return true;
		}

		// No valid Arena .exe found.
		return false;
	}
}

Game::Game()
{
	this->physicsTempAllocator = nullptr;

	// Keeps us from deleting a sub-panel the same frame it's in use. The pop is delayed until the
	// beginning of the next frame.
	this->requestedSubPanelPop = false;

	this->shouldSimulateScene = false;
	this->shouldRenderScene = false;
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

	if (this->renderTargetsResetListenerID.has_value())
	{
		this->inputManager.removeListener(*this->renderTargetsResetListenerID);
	}

	if (this->takeScreenshotListenerID.has_value())
	{
		this->inputManager.removeListener(*this->takeScreenshotListenerID);
	}

	if (this->debugProfilerListenerID.has_value())
	{
		this->inputManager.removeListener(*this->debugProfilerListenerID);
	}

	this->sceneManager.renderVoxelChunkManager.shutdown(this->renderer);
	this->sceneManager.renderEntityChunkManager.shutdown(this->renderer);
	this->sceneManager.renderLightChunkManager.shutdown(this->renderer);
	this->sceneManager.renderSkyManager.shutdown(this->renderer);
	this->sceneManager.renderWeatherManager.shutdown(this->renderer);
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

	// Search ArenaPaths directories for a valid Arena install.
	const std::string &arenaPathsString = this->options.getMisc_ArenaPaths();
	const Buffer<std::string> arenaPaths = String::split(arenaPathsString, ',');
	std::string arenaPath;
	bool isFloppyDiskVersion;
	if (!TryGetArenaAssetsDirectory(arenaPaths, basePath, &arenaPath, &isFloppyDiskVersion))
	{
		DebugLogError("Couldn't find Arena executable in these directories: " + arenaPathsString);
		return false;
	}

	VFS::Manager::get().initialize(std::string(arenaPath));

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
		return this->options.getGraphics_ResolutionScale();
	};

	constexpr RendererSystemType2D rendererSystemType2D = RendererSystemType2D::SDL2;
	constexpr RendererSystemType3D rendererSystemType3D = RendererSystemType3D::SoftwareClassic;
	const DitheringMode ditheringMode = static_cast<DitheringMode>(this->options.getGraphics_DitheringMode());
	if (!this->renderer.init(this->options.getGraphics_ScreenWidth(), this->options.getGraphics_ScreenHeight(),
		static_cast<Renderer::WindowMode>(this->options.getGraphics_WindowMode()), this->options.getGraphics_LetterboxMode(),
		this->options.getGraphics_ModernInterface(), resolutionScaleFunc, rendererSystemType2D, rendererSystemType3D,
		this->options.getGraphics_RenderThreadsMode(), ditheringMode))
	{
		DebugLogError("Couldn't init renderer (2D: " + std::to_string(static_cast<int>(rendererSystemType2D)) +
			", 3D: " + std::to_string(static_cast<int>(rendererSystemType3D)) + ").");
		return false;
	}

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

	this->renderTargetsResetListenerID = this->inputManager.addRenderTargetsResetListener(
		[this]()
	{
		this->renderer.handleRenderTargetsReset();
	});

	this->takeScreenshotListenerID = this->inputManager.addInputActionListener(InputActionName::Screenshot,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			const Surface screenshot = this->renderer.getScreenshot();
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

	const std::string clockLibraryPath = dataFolderPath + "Clocks.txt";
	if (!ClockLibrary::getInstance().init(clockLibraryPath.c_str()))
	{
		DebugLogError("Couldn't init clock library with path \"" + clockLibraryPath + "\".");
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

	this->sceneManager.init(this->textureManager, this->renderer);
	this->sceneManager.renderVoxelChunkManager.init(this->renderer);
	this->sceneManager.renderEntityChunkManager.init(this->renderer);
	this->sceneManager.renderLightChunkManager.init(this->renderer);
	this->sceneManager.renderSkyManager.init(exeData, this->textureManager, this->renderer);

	if (!this->sceneManager.renderWeatherManager.init(this->renderer))
	{
		DebugLogError("Couldn't init render weather manager.");
		return false;
	}

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

bool Game::characterCreationIsActive() const
{
	return this->charCreationState.get() != nullptr;
}

CharacterCreationState &Game::getCharacterCreationState() const
{
	DebugAssert(this->characterCreationIsActive());
	return *this->charCreationState.get();
}

const Rect &Game::getNativeCursorRegion(int index) const
{
	DebugAssertIndex(this->nativeCursorRegions, index);
	return this->nativeCursorRegions[index];
}

TextBox *Game::getTriggerTextBox()
{
	DebugAssert(this->shouldSimulateScene);
	DebugAssert(this->gameState.isActiveMapValid());

	Panel *panel = this->getActivePanel();
	if (panel == nullptr)
	{
		DebugLogError("No active panel for trigger text box getter.");
		return nullptr;
	}

	GameWorldPanel *gameWorldPanel = static_cast<GameWorldPanel*>(panel); // @todo: can't use dynamic_cast anymore, this isn't safe.
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
	this->renderer.resize(windowWidth, windowHeight, this->options.getGraphics_ResolutionScale(), fullGameWindow);

	// Update where the mouse can click for player movement in the classic interface.
	this->updateNativeCursorRegions(windowWidth, windowHeight);

	if (this->gameState.isActiveMapValid())
	{
		// Update frustum culling in case the aspect ratio widens while there's a game world pop-up.
		const CoordDouble3 playerCoord = this->player.getEyeCoord();
		const RenderCamera renderCamera = RendererUtils::makeCamera(playerCoord.chunk, playerCoord.point, this->player.forward,
			this->options.getGraphics_VerticalFOV(), this->renderer.getViewAspect(), this->options.getGraphics_TallPixelCorrection());
		this->gameState.tickVisibility(renderCamera, *this);
		this->gameState.tickRendering(renderCamera, *this);
	}
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
	GameWorldUiModel::updateNativeCursorRegions(this->nativeCursorRegions, windowWidth, windowHeight);
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
			const std::string renderTime = String::fixedPrecision(profilerData.renderTime * 1000.0, 2);
			const std::string presentTime = String::fixedPrecision(profilerData.presentTime * 1000.0, 2);
			const std::string renderDrawCallCount = std::to_string(profilerData.drawCallCount);
			const std::string renderDepthTestRatio = String::fixedPrecision(static_cast<double>(profilerData.totalDepthTests) / static_cast<double>(profilerData.pixelCount), 2);
			const std::string renderColorOverdrawRatio = String::fixedPrecision(static_cast<double>(profilerData.totalColorWrites) / static_cast<double>(profilerData.pixelCount), 2);
			const std::string objectTextureMbCount = String::fixedPrecision(static_cast<double>(profilerData.objectTextureByteCount) / (1024.0 * 1024.0), 2);
			debugText.append("\nRender: " + renderWidth + "x" + renderHeight + " (" + renderResScale + "), " +
				renderThreadCount + " thread" + ((profilerData.threadCount > 1) ? "s" : "") + '\n' +
				"3D render: " + renderTime + "ms" + '\n' +
				"Present: " + presentTime + "ms" + '\n' +
				"Textures: " + std::to_string(profilerData.objectTextureCount) + " (" + objectTextureMbCount + "MB)" + '\n' +
				"Draw calls: " + renderDrawCallCount + '\n' +
				"Rendered Tris: " + std::to_string(profilerData.presentedTriangleCount) + '\n' +
				"Lights: " + std::to_string(profilerData.totalLightCount) + '\n' +
				"Depth tests: " + renderDepthTestRatio + "x" + '\n' +
				"Overdraw: " + renderColorOverdrawRatio + "x");
		}
		else
		{
			debugText.append("\nNo profiler data available.");
		}
	}

	if (profilerLevel >= 3)
	{
		// Player position, direction, etc.
		const CoordDouble3 playerCoord = this->player.getEyeCoord();
		const Double3 &direction = this->player.forward;

		const std::string chunkStr = playerCoord.chunk.toString();
		const std::string chunkPosX = String::fixedPrecision(playerCoord.point.x, 2);
		const std::string chunkPosY = String::fixedPrecision(playerCoord.point.y, 2);
		const std::string chunkPosZ = String::fixedPrecision(playerCoord.point.z, 2);
		const std::string dirX = String::fixedPrecision(direction.x, 2);
		const std::string dirY = String::fixedPrecision(direction.y, 2);
		const std::string dirZ = String::fixedPrecision(direction.z, 2);

		debugText.append("\nChunk: " + chunkStr + '\n' +
			"Chunk pos: " + chunkPosX + ", " + chunkPosY + ", " + chunkPosZ + '\n' +
			"Dir: " + dirX + ", " + dirY + ", " + dirZ);

		// Set Jolt Physics camera position for LOD.
		const WorldDouble3 playerWorldPos = VoxelUtils::coordToWorldPoint(playerCoord);
		this->renderer.SetCameraPos(JPH::RVec3Arg(static_cast<float>(playerWorldPos.x), static_cast<float>(playerWorldPos.y), static_cast<float>(playerWorldPos.z)));

		JPH::BodyManager::DrawSettings drawSettings;
		this->physicsSystem.DrawBodies(drawSettings, &this->renderer);

		GameWorldUiView::DEBUG_DrawVoxelVisibilityQuadtree(*this);
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
	// Set up physics system values.
	JPH::TempAllocatorImpl physicsAllocator(Physics::TempAllocatorByteCount);
	JPH::JobSystemThreadPool physicsJobThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, Physics::ThreadCount); // @todo: implement own derived JobSystem class
	PhysicsBroadPhaseLayerInterface physicsBroadPhaseLayerInterface;
	PhysicsObjectVsBroadPhaseLayerFilter physicsObjectVsBroadPhaseLayerFilter;
	PhysicsObjectLayerPairFilter physicsObjectLayerPairFilter;
	
	this->physicsTempAllocator = &physicsAllocator;
	this->physicsSystem.Init(Physics::MaxBodies, Physics::BodyMutexCount, Physics::MaxBodyPairs, Physics::MaxContactConstraints, physicsBroadPhaseLayerInterface, physicsObjectVsBroadPhaseLayerFilter, physicsObjectLayerPairFilter);

	PhysicsBodyActivationListener physicsBodyActivationListener;
	PhysicsContactListener physicsContactListener;
	this->physicsSystem.SetBodyActivationListener(&physicsBodyActivationListener);
	this->physicsSystem.SetContactListener(&physicsContactListener);

	// Initialize panel and music to default (bootstrapping the first game frame).
	this->panel = IntroUiModel::makeStartupPanel(*this);

	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *mainMenuMusicDef = musicLibrary.getRandomMusicDefinition(MusicType::MainMenu, this->random);
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

	FrameTimer frameTimer;
	frameTimer.init();

	// Primary game loop.
	while (this->running)
	{
		frameTimer.startFrame(this->options.getGraphics_TargetFPS());
		const double deltaTime = frameTimer.deltaTime;
		const double clampedDeltaTime = frameTimer.clampedDeltaTime;

		Profiler::startFrame();

		this->fpsCounter.updateFrameTime(deltaTime);

		// User input.
		try
		{
			const BufferView<const ButtonProxy> buttonProxies = this->getActivePanel()->getButtonProxies();
			auto onFinishedProcessingEventFunc = [this]()
			{
				this->handlePanelChanges();
			};

			this->inputManager.update(*this, deltaTime, buttonProxies, onFinishedProcessingEventFunc);

			if (this->shouldSimulateScene)
			{
				const Double2 playerTurnAngleDeltas = PlayerLogicController::makeTurningAngularValues(*this, clampedDeltaTime, this->nativeCursorRegions);

				// Multiply by 100 so the values in options are more convenient.
				const Degrees deltaDegreesX = playerTurnAngleDeltas.x * (100.0 * this->options.getInput_HorizontalSensitivity());
				const Degrees deltaDegreesY = playerTurnAngleDeltas.y * (100.0 * this->options.getInput_VerticalSensitivity());
				const Degrees pitchLimit = this->options.getInput_CameraPitchLimit();
				this->player.rotateX(deltaDegreesX);
				this->player.rotateY(deltaDegreesY, pitchLimit);
				PlayerLogicController::handlePlayerMovement(*this, clampedDeltaTime, this->nativeCursorRegions);
			}
		}
		catch (const std::exception &e)
		{
			DebugCrash("User input exception: " + std::string(e.what()));
		}

		// Tick game state.
		try
		{
			this->getActivePanel()->tick(clampedDeltaTime);
			this->handlePanelChanges();

			if (this->shouldSimulateScene && this->gameState.isActiveMapValid())
			{
				const CoordDouble3 oldPlayerCoord = this->player.getEyeCoord();
				const int chunkDistance = this->options.getMisc_ChunkDistance();
				ChunkManager &chunkManager = this->sceneManager.chunkManager;
				chunkManager.update(oldPlayerCoord.chunk, chunkDistance);

				this->gameState.tickGameClock(clampedDeltaTime, *this);
				this->gameState.tickChasmAnimation(clampedDeltaTime);
				this->gameState.tickSky(clampedDeltaTime, *this);
				this->gameState.tickWeather(clampedDeltaTime, *this);
				this->gameState.tickUiMessages(clampedDeltaTime);
				this->gameState.tickPlayerAttack(clampedDeltaTime, *this);
				this->gameState.tickVoxels(clampedDeltaTime, *this);
				this->gameState.tickEntities(clampedDeltaTime, *this);
				this->gameState.tickCollision(clampedDeltaTime, this->physicsSystem, *this);

				this->player.prePhysicsStep(clampedDeltaTime, *this);
				this->physicsSystem.Update(static_cast<float>(clampedDeltaTime), frameTimer.physicsSteps, &physicsAllocator, &physicsJobThreadPool);
				this->player.postPhysicsStep(*this);

				const CoordDouble3 newPlayerCoord = this->player.getEyeCoord();
				this->gameState.tickPlayerMovementTriggers(oldPlayerCoord, newPlayerCoord, *this);

				const Double3 newPlayerDirection = this->player.forward;
				const RenderCamera renderCamera = RendererUtils::makeCamera(newPlayerCoord.chunk, newPlayerCoord.point, newPlayerDirection,
					this->options.getGraphics_VerticalFOV(), this->renderer.getViewAspect(), this->options.getGraphics_TallPixelCorrection());

				this->gameState.tickVisibility(renderCamera, *this);
				this->gameState.tickRendering(renderCamera, *this);

				// Update audio listener orientation.
				const WorldDouble3 newPlayerWorldPos = VoxelUtils::coordToWorldPoint(newPlayerCoord);
				const AudioManager::ListenerData listenerData(newPlayerWorldPos, newPlayerDirection);
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
			if (this->gameState.hasPendingSceneChange())
			{
				this->gameState.applyPendingSceneChange(*this, this->physicsSystem, clampedDeltaTime);
			}
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

			if (this->shouldRenderScene)
			{
				if (!GameWorldPanel::renderScene(*this))
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
			this->sceneManager.cleanUp(this->physicsSystem);
		}
		catch (const std::exception &e)
		{
			DebugCrash("Clean-up exception: " + std::string(e.what()));
		}
	}

	// At this point, the engine has received an exit signal and is now quitting peacefully.
	this->player.freePhysicsBody(this->physicsSystem);
	this->sceneManager.collisionChunkManager.clear(this->physicsSystem);

	this->options.saveChanges();
}
