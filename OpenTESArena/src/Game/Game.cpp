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

#include "Game.h"
#include "Options.h"
#include "../Assets/ArenaLevelLibrary.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Assets/TextureManager.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/SoundLibrary.h"
#include "../Collision/Physics.h"
#include "../Collision/PhysicsBodyActivationListener.h"
#include "../Collision/PhysicsContactListener.h"
#include "../Collision/PhysicsLayer.h"
#include "../Entities/EntityAnimationLibrary.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Input/InputActionName.h"
#include "../Interface/CinematicLibrary.h"
#include "../Interface/CommonUiController.h"
#include "../Interface/CommonUiView.h"
#include "../Interface/GameWorldPanel.h"
#include "../Interface/GameWorldUiModel.h"
#include "../Interface/GameWorldUiView.h"
#include "../Interface/IntroUiModel.h"
#include "../Interface/MainMenuPanel.h"
#include "../Interface/Panel.h"
#include "../Items/ItemConditionLibrary.h"
#include "../Items/ItemLibrary.h"
#include "../Items/ItemMaterialLibrary.h"
#include "../Player/PlayerInterface.h"
#include "../Player/PlayerLogic.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Rendering/RenderBackendType.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/RenderCommand.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"
#include "../Rendering/RenderFrameSettings.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Time/ClockLibrary.h"
#include "../UI/FontLibrary.h"
#include "../UI/GuiUtils.h"
#include "../UI/Surface.h"
#include "../UI/UiCommand.h"
#include "../UI/UiContext.h"
#include "../Utilities/Platform.h"
#include "../World/MapLogic.h"
#include "../World/MapType.h"
#include "../World/MeshLibrary.h"

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
		std::chrono::nanoseconds maximumFrameDuration; // Longest allowed frame time before engine will run in slow motion.
		std::chrono::nanoseconds minimumFrameDuration; // Shortest allowed frame time if not enough work is happening.
		std::chrono::time_point<std::chrono::high_resolution_clock> previousTimePoint, currentTimePoint;
		double deltaTime; // Difference between frame times in seconds.
		double clampedDeltaTime; // For game logic calculations that become imprecise or break at low FPS.
		int physicsSteps; // 1 unless the engine has to do more steps this frame to keep numeric accuracy.

		FrameTimer()
		{
			this->maximumFrameDuration = std::chrono::nanoseconds(0);
			this->minimumFrameDuration = std::chrono::nanoseconds(0);
			this->deltaTime = 0.0;
			this->clampedDeltaTime = 0.0;
			this->physicsSteps = 0;
		}

		void init()
		{
			this->maximumFrameDuration = std::chrono::nanoseconds(std::nano::den / Options::MIN_FPS);
			this->currentTimePoint = std::chrono::high_resolution_clock::now();
		}

		void startFrame(int targetFPS)
		{
			DebugAssert(targetFPS > 0);
			this->minimumFrameDuration = std::chrono::nanoseconds(std::nano::den / targetFPS);
			this->previousTimePoint = this->currentTimePoint;
			this->currentTimePoint = std::chrono::high_resolution_clock::now();

			auto previousFrameDuration = this->currentTimePoint - this->previousTimePoint;
			if (previousFrameDuration < this->minimumFrameDuration)
			{
				const auto sleepBias = previousFrameDuration / 1000; // Keep slightly above target FPS instead of slightly below.
				const auto sleepDuration = this->minimumFrameDuration - previousFrameDuration - sleepBias;
				const auto reducedSleepDuration = (sleepDuration * 5) / 10; // Sleep less to prevent oversleeping, busy wait the rest.
				std::this_thread::sleep_for(reducedSleepDuration);

				while (true)
				{
					const auto timePointWhileBusyWaiting = std::chrono::high_resolution_clock::now();
					if (timePointWhileBusyWaiting - this->currentTimePoint > sleepDuration)
					{
						break;
					}

					std::this_thread::yield();
				}

				this->currentTimePoint = std::chrono::high_resolution_clock::now();
				previousFrameDuration = this->currentTimePoint - this->previousTimePoint;
			}

			constexpr double timeUnitsReal = static_cast<double>(std::nano::den);
			this->deltaTime = static_cast<double>(previousFrameDuration.count()) / timeUnitsReal;
			this->clampedDeltaTime = std::fmin(previousFrameDuration.count(), this->maximumFrameDuration.count()) / timeUnitsReal;
			this->physicsSteps = static_cast<int>(std::ceil(this->clampedDeltaTime / Physics::DeltaTime));
		}
	};

	bool TryGetArenaAssetsDirectory(Span<const std::string> arenaPaths, const std::string &basePath, std::string *outDirectory, bool *outIsFloppyDiskVersion)
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

	this->cursorImageElementInstID = -1;

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

	if (this->defaultCursorTextureID >= 0)
	{
		this->renderer.freeUiTexture(this->defaultCursorTextureID);
		this->defaultCursorTextureID = -1;
	}

	if (this->cursorImageElementInstID >= 0)
	{
		this->uiManager.freeImage(this->cursorImageElementInstID);
		this->cursorImageElementInstID = -1;
	}

	this->uiManager.shutdown(this->renderer);
	this->sceneManager.shutdown(this->renderer);

	this->debugQuadtreeState.free(this->renderer);
}

bool Game::init()
{
	DebugLogFormat("Initializing (Platform: %s).", Platform::getPlatform().c_str());

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

	const bool midiPathIsRelative = Path::isRelative(this->options.getAudio_MidiConfig().c_str());
	const std::string midiFilePath = (midiPathIsRelative ? basePath : "") + this->options.getAudio_MidiConfig();
	const std::string audioDataPath = dataFolderPath + "audio/";
	this->audioManager.init(this->options.getAudio_MusicVolume(), this->options.getAudio_SoundVolume(),
		this->options.getAudio_SoundChannels(), this->options.getAudio_SoundResampling(),
		this->options.getAudio_Is3DAudio(), midiFilePath, audioDataPath);

	const RenderBackendType renderBackendType = static_cast<RenderBackendType>(this->options.getGraphics_GraphicsAPI());
	const uint32_t windowAdditionalFlags = (renderBackendType == RenderBackendType::Vulkan) ? SDL_WINDOW_VULKAN : 0;
	if (!this->window.init(this->options.getGraphics_ScreenWidth(), this->options.getGraphics_ScreenHeight(), 
		static_cast<RenderWindowMode>(this->options.getGraphics_WindowMode()), windowAdditionalFlags, this->options.getGraphics_LetterboxMode(),
		this->options.getGraphics_ModernInterface()))
	{
		DebugLogErrorFormat("Couldn't init window.");
		return false;
	}

	auto resolutionScaleFunc = [this]()
	{
		return this->options.getGraphics_ResolutionScale();
	};

	const int renderThreadsMode = this->options.getGraphics_RenderThreadsMode();
	const DitheringMode ditheringMode = static_cast<DitheringMode>(this->options.getGraphics_DitheringMode());
	const bool enableValidationLayers = this->options.getMisc_EnableValidationLayers();
	if (!this->renderer.init(&this->window, renderBackendType, resolutionScaleFunc, renderThreadsMode, ditheringMode, enableValidationLayers, dataFolderPath))
	{
		DebugLogErrorFormat("Couldn't init renderer.");
		return false;
	}

	const double logicalToPixelScale = this->window.getLogicalToPixelScale();
	this->inputManager.init(logicalToPixelScale);

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

	const std::string meshLibraryPath = dataFolderPath + "meshes/";
	if (!MeshLibrary::getInstance().init(meshLibraryPath.c_str()))
	{
		DebugLogError("Couldn't init mesh library.");
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

	SoundLibrary::getInstance().init();

	const std::string musicLibraryPath = audioDataPath + "MusicDefinitions.txt";
	if (!MusicLibrary::getInstance().init(musicLibraryPath.c_str()))
	{
		DebugLogError("Couldn't init music library with path \"" + musicLibraryPath + "\".");
		return false;
	}

	CinematicLibrary::getInstance().init();

	const ExeData &exeData = binaryAssetLibrary.getExeData();
	ItemConditionLibrary::getInstance().init(exeData);
	ItemMaterialLibrary::getInstance().init(exeData);
	ItemLibrary::getInstance().init(exeData);
	WeaponAnimationLibrary::getInstance().init(exeData, this->textureManager);

	CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	charClassLibrary.init(exeData);
	CharacterRaceLibrary::getInstance().init(exeData);

	EntityAnimationLibrary &entityAnimLibrary = EntityAnimationLibrary::getInstance();
	entityAnimLibrary.init(binaryAssetLibrary, charClassLibrary, this->textureManager);
	EntityDefinitionLibrary::getInstance().init(exeData, charClassLibrary, entityAnimLibrary);

	this->sceneManager.init(this->textureManager, this->renderer);
	this->sceneManager.renderVoxelChunkManager.init(this->renderer);
	this->sceneManager.renderEntityManager.init(this->renderer);
	this->sceneManager.renderSkyManager.init(exeData, this->textureManager, this->renderer);

	if (!this->sceneManager.renderWeatherManager.init(this->textureManager, this->renderer))
	{
		DebugLogError("Couldn't init render weather manager.");
		return false;
	}

	if (!this->sceneManager.renderLightManager.init(this->renderer))
	{
		DebugLogError("Couldn't init render light manager.");
		return false;
	}

	const std::string uiFolderPath = dataFolderPath + "ui/";
	if (!this->uiManager.init(uiFolderPath.c_str(), this->textureManager, this->renderer))
	{
		DebugLogError("Couldn't init UI manager.");
		return false;
	}

	this->defaultCursorTextureID = CommonUiView::allocDefaultCursorTexture(this->textureManager, this->renderer);

	UiElementInitInfo cursorImageElementInitInfo;
	cursorImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
	cursorImageElementInitInfo.contextType = UiContextType::Global;
	cursorImageElementInitInfo.drawOrder = 100;
	cursorImageElementInitInfo.renderSpace = UiRenderSpace::Native;
	this->cursorImageElementInstID = this->uiManager.createImage(cursorImageElementInitInfo, this->defaultCursorTextureID);

	// Initialize window icon.
	const std::string windowIconPath = dataFolderPath + "icon.bmp";
	const Surface windowIconSurface = Surface::loadBMP(windowIconPath.c_str(), RendererUtils::DEFAULT_PIXELFORMAT);
	if (windowIconSurface.get() == nullptr)
	{
		DebugLogError("Couldn't load window icon with path \"" + windowIconPath + "\".");
		return false;
	}

	const uint32_t windowIconColorKey = windowIconSurface.mapRGBA(0, 0, 0, 255);
	SDL_SetColorKey(windowIconSurface.get(), SDL_TRUE, windowIconColorKey);
	this->window.setIcon(windowIconSurface);

	// Initialize click regions for player movement in classic interface mode.
	const Int2 windowDims = this->window.getPixelDimensions();
	this->updateNativeCursorRegions(windowDims.x, windowDims.y);

	// Random seed.
	this->random.init();

	// Initialize debug display.
	const TextBoxInitInfo debugInfoTextBoxInitInfo = CommonUiView::getDebugInfoTextBoxInitInfo(FontLibrary::getInstance());
	if (!this->debugInfoTextBox.init(debugInfoTextBoxInitInfo, this->renderer))
	{
		DebugLogError("Couldn't init debug info text box.");
		return false;
	}

	this->debugQuadtreeState = GameWorldUiView::allocDebugVoxelVisibilityQuadtreeState(this->renderer);

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

TextBox *Game::getActionTextBox()
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
	return &gameWorldPanel->getActionTextBox();
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
	this->renderer.resize(windowWidth, windowHeight);

	// Update where the mouse can click for player movement in the classic interface.
	this->updateNativeCursorRegions(windowWidth, windowHeight);

	if (this->gameState.isActiveMapValid())
	{
		// Update frustum culling in case the aspect ratio widens while there's a game world pop-up.
		const WorldDouble3 playerPosition = this->player.getEyePosition();
		const double tallPixelRatio = RendererUtils::getTallPixelRatio(this->options.getGraphics_TallPixelCorrection());
		
		RenderCamera renderCamera;
		renderCamera.init(playerPosition, this->player.angleX, this->player.angleY, this->options.getGraphics_VerticalFOV(), this->window.getSceneViewAspectRatio(), tallPixelRatio);

		constexpr bool isFloatingOriginChanged = false;

		this->gameState.tickVisibility(renderCamera, *this);
		this->gameState.tickRendering(0.0, renderCamera, isFloatingOriginChanged, *this);
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

void Game::updateDebugInfoText()
{
	const int profilerLevel = this->options.getMisc_ProfilerLevel();

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
		debugText.append("FPS: " + averageFpsText + " (" + averageFrameTimeText + "ms " + lowestFrameTimeText + "ms " + highestFrameTimeText + "ms)");
	}

	const Int2 windowDims = this->window.getPixelDimensions();
	if (profilerLevel >= 2)
	{
		// Renderer details (window res, render res, threads, frame times, etc.).
		const std::string windowWidth = std::to_string(windowDims.x);
		const std::string windowHeight = std::to_string(windowDims.y);
		debugText.append("\nWindow: " + windowWidth + "x" + windowHeight);

		const RendererProfilerData &profilerData = this->renderer.getProfilerData();
		const Int2 renderDims(profilerData.width, profilerData.height);
		const bool profilerDataIsValid = (renderDims.x > 0) && (renderDims.y > 0);
		if (profilerDataIsValid)
		{
			const double resolutionScale = this->options.getGraphics_ResolutionScale();
			const std::string renderWidth = std::to_string(renderDims.x);
			const std::string renderHeight = std::to_string(renderDims.y);
			const std::string renderResScale = String::fixedPrecision(resolutionScale * 100.0, 0) + "%";
			const std::string renderThreadCount = std::to_string(profilerData.threadCount);
			const std::string renderTime = String::fixedPrecision(profilerData.renderTime * 1000.0, 2);
			const std::string renderDrawCallCount = std::to_string(profilerData.drawCallCount);
			const std::string renderCoverageTestRatio = String::fixedPrecision(static_cast<double>(profilerData.totalCoverageTests) / static_cast<double>(profilerData.pixelCount), 2);
			const std::string renderDepthTestRatio = String::fixedPrecision(static_cast<double>(profilerData.totalDepthTests) / static_cast<double>(profilerData.pixelCount), 2);
			const std::string renderColorOverdrawRatio = String::fixedPrecision(static_cast<double>(profilerData.totalColorWrites) / static_cast<double>(profilerData.pixelCount), 2);
			const std::string objectTextureMbCount = String::fixedPrecision(static_cast<double>(profilerData.objectTextureByteCount) / (1024.0 * 1024.0), 2);
			const std::string uiTextureMbCount = String::fixedPrecision(static_cast<double>(profilerData.uiTextureByteCount) / (1024.0 * 1024.0), 2);
			debugText.append("\nScene: " + renderWidth + "x" + renderHeight + " (" + renderResScale + ")" + '\n' +
				"Render: " + renderTime + "ms, " + renderThreadCount + " thread" + ((profilerData.threadCount > 1) ? "s" : "") + '\n' +
				"Object textures: " + std::to_string(profilerData.objectTextureCount) + " (" + objectTextureMbCount + "MB)" + '\n' +
				"UI textures: " + std::to_string(profilerData.uiTextureCount) + " (" + uiTextureMbCount + "MB)" + '\n' +
				"Materials: " + std::to_string(profilerData.materialCount) + '\n' +
				"Draw calls: " + renderDrawCallCount + '\n' +
				"Rendered Tris: " + std::to_string(profilerData.presentedTriangleCount) + '\n' +
				"Lights: " + std::to_string(profilerData.totalLightCount) + '\n' +
				"Coverage tests: " + renderCoverageTestRatio + "x" + '\n' +
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

		if (this->shouldRenderScene)
		{
			// Set Jolt Physics camera position for LOD.
			/*const WorldDouble3 playerWorldPos = VoxelUtils::coordToWorldPoint(playerCoord);
			this->renderer.SetCameraPos(JPH::RVec3Arg(static_cast<float>(playerWorldPos.x), static_cast<float>(playerWorldPos.y), static_cast<float>(playerWorldPos.z)));

			JPH::BodyManager::DrawSettings drawSettings;
			this->physicsSystem.DrawBodies(drawSettings, &this->renderer);*/
		}
	}

	this->debugInfoTextBox.setText(debugText);
}

void Game::loop()
{
	// Set up physics system values.
	JPH::TempAllocatorImpl physicsAllocator(Physics::TempAllocatorByteCount);
	this->physicsTempAllocator = &physicsAllocator;
	
	PhysicsBroadPhaseLayerInterface physicsBroadPhaseLayerInterface;
	PhysicsObjectVsBroadPhaseLayerFilter physicsObjectVsBroadPhaseLayerFilter;
	PhysicsObjectLayerPairFilter physicsObjectLayerPairFilter;	
	this->physicsSystem.Init(Physics::MaxBodies, Physics::BodyMutexCount, Physics::MaxBodyPairs, Physics::MaxContactConstraints, physicsBroadPhaseLayerInterface, physicsObjectVsBroadPhaseLayerFilter, physicsObjectLayerPairFilter);

	PhysicsBodyActivationListener physicsBodyActivationListener;
	PhysicsContactListener physicsContactListener(*this);
	this->physicsSystem.SetBodyActivationListener(&physicsBodyActivationListener);
	this->physicsSystem.SetContactListener(&physicsContactListener);

	JPH::JobSystemThreadPool physicsJobThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, Physics::ThreadCount); // @todo: implement own derived JobSystem class

	// Initialize panel and music to default (bootstrapping the first game frame).
	this->panel = IntroUiModel::makeStartupPanel(*this);

	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *mainMenuMusicDef = musicLibrary.getRandomMusicDefinition(MusicType::MainMenu, this->random);
	if (mainMenuMusicDef == nullptr)
	{
		DebugLogWarning("Missing main menu music.");
	}

	this->audioManager.setMusic(mainMenuMusicDef);

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
			const Span<const ButtonProxy> buttonProxies = this->getActivePanel()->getButtonProxies();
			auto onFinishedProcessingEventFunc = [this]()
			{
				this->handlePanelChanges();
			};

			this->inputManager.update(*this, deltaTime, buttonProxies, onFinishedProcessingEventFunc);

			if (this->shouldSimulateScene && this->gameState.isActiveMapValid())
			{
				const Double2 playerTurnAngleDeltas = PlayerLogic::makeTurningAngularValues(*this, clampedDeltaTime, this->inputManager.getMouseDelta(), this->nativeCursorRegions);

				// Multiply by 100 so the values in options are more convenient.
				const Degrees deltaDegreesX = playerTurnAngleDeltas.x * (100.0 * this->options.getInput_HorizontalSensitivity());
				const Degrees deltaDegreesY = playerTurnAngleDeltas.y * (100.0 * this->options.getInput_VerticalSensitivity());
				const bool invertVerticalAxis = this->options.getInput_InvertVerticalAxis();
				const double verticalAxisSign = invertVerticalAxis ? -1.0 : 1.0;
				const Degrees pitchLimit = this->options.getInput_CameraPitchLimit();
				this->player.rotateX(deltaDegreesX);
				this->player.rotateY(deltaDegreesY * verticalAxisSign, pitchLimit);
				
				if (this->player.movementType == PlayerMovementType::Climbing)
				{
					// Have to keep pushing every frame to keep from falling.
					this->player.climbingState.isAccelerationValidForClimbing = false;
				}
				
				const PlayerInputAcceleration inputAcceleration = PlayerLogic::getInputAcceleration(*this, this->nativeCursorRegions);
				if (inputAcceleration.shouldResetVelocity)
				{
					this->player.setPhysicsVelocity(Double3::Zero);
				}

				if (inputAcceleration.isGhostMode)
				{
					const WorldDouble3 oldPlayerFeetPosition = this->player.getFeetPosition();
					const WorldDouble3 newPlayerFeetPosition = oldPlayerFeetPosition + (inputAcceleration.direction * (inputAcceleration.magnitude * clampedDeltaTime));
					this->player.setPhysicsPositionRelativeToFeet(newPlayerFeetPosition);
				}
				else if (inputAcceleration.isInstantJump)
				{
					this->player.setPhysicsVelocityY(inputAcceleration.magnitude);
				}
				else if (inputAcceleration.direction.isNormalized())
				{
					this->player.accelerate(inputAcceleration.direction, inputAcceleration.magnitude, clampedDeltaTime);
				}
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
			this->uiManager.update(clampedDeltaTime, *this);

			const Int2 cursorPosition = this->inputManager.getMousePosition();
			this->uiManager.setTransformPosition(this->cursorImageElementInstID, cursorPosition);

			if (this->shouldSimulateScene && this->gameState.isActiveMapValid())
			{
				const WorldDouble3 oldPlayerPosition = this->player.getEyePosition();
				const ChunkInt2 oldPlayerChunk = VoxelUtils::worldPointToChunk(oldPlayerPosition);
				const int chunkDistance = this->options.getMisc_ChunkDistance();
				ChunkManager &chunkManager = this->sceneManager.chunkManager;
				chunkManager.update(oldPlayerChunk, chunkDistance);

				this->gameState.tickGameClock(clampedDeltaTime, *this);
				this->gameState.tickChasmAnimation(clampedDeltaTime);
				this->gameState.tickSky(clampedDeltaTime, *this);
				this->gameState.tickWeather(clampedDeltaTime, *this);
				this->gameState.tickUiMessages(clampedDeltaTime);
				this->gameState.tickPlayerHealth(clampedDeltaTime, *this);
				this->gameState.tickPlayerStamina(clampedDeltaTime, *this);
				this->gameState.tickPlayerAttack(clampedDeltaTime, *this);				
				this->gameState.tickVoxels(clampedDeltaTime, *this);
				this->gameState.tickEntities(clampedDeltaTime, *this);
				this->gameState.tickCollision(clampedDeltaTime, this->physicsSystem, *this);

				this->player.prePhysicsStep(clampedDeltaTime, *this);
				this->physicsSystem.Update(static_cast<float>(clampedDeltaTime), frameTimer.physicsSteps, &physicsAllocator, &physicsJobThreadPool);
				this->player.postPhysicsStep(clampedDeltaTime, *this);

				if (this->gameState.hasPendingLevelTransitionCalculation())
				{
					MapLogic::handleInteriorLevelTransition(*this, this->gameState.getLevelTransitionCalculationPlayerCoord(), this->gameState.getLevelTransitionCalculationTransitionCoord());
					this->gameState.clearLevelTransitionCalculation();
				}

				const WorldDouble3 newPlayerPosition = this->player.getEyePosition();
				const ChunkInt2 newPlayerChunk = VoxelUtils::worldPointToChunk(newPlayerPosition);
				const Degrees newPlayerYaw = this->player.angleX;
				const Degrees newPlayerPitch = this->player.angleY;
				const double tallPixelRatio = RendererUtils::getTallPixelRatio(this->options.getGraphics_TallPixelCorrection());
				RenderCamera renderCamera;
				renderCamera.init(newPlayerPosition, newPlayerYaw, newPlayerPitch, this->options.getGraphics_VerticalFOV(), this->window.getSceneViewAspectRatio(), tallPixelRatio);

				bool isFloatingOriginChanged = newPlayerChunk != oldPlayerChunk;
				if (this->options.getMisc_GhostMode())
				{
					// @temp hack due to how ghost mode skips character post-simulation (causing PhysicsSystem::Update() to not affect player).
					isFloatingOriginChanged = true;
				}

				this->gameState.tickVisibility(renderCamera, *this);
				this->gameState.tickRendering(clampedDeltaTime, renderCamera, isFloatingOriginChanged, *this);

				// Update audio listener orientation.
				const AudioListenerState listenerState(newPlayerPosition, this->player.forward, this->player.up);
				this->audioManager.updateListener(listenerState);
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
			RenderCommandList renderCommandList;
			UiCommandList uiCommandList;
			RenderCamera renderCamera;
			RenderFrameSettings frameSettings;

			if (this->shouldRenderScene)
			{
				const RenderSkyManager &renderSkyManager = this->sceneManager.renderSkyManager;
				renderSkyManager.populateCommandList(renderCommandList);

				this->sceneManager.renderVoxelChunkManager.populateCommandList(renderCommandList);
				this->sceneManager.renderEntityManager.populateCommandList(renderCommandList);

				const WeatherInstance &activeWeatherInst = this->gameState.getWeatherInstance();
				const bool isFoggy = this->gameState.isFogActive();
				this->sceneManager.renderWeatherManager.populateCommandList(renderCommandList, activeWeatherInst, isFoggy);

				const MapDefinition &activeMapDef = this->gameState.getActiveMapDef();
				const MapType activeMapType = activeMapDef.getMapType();
				const double ambientPercent = ArenaRenderUtils::getAmbientPercent(this->gameState.getClock(), activeMapType, isFoggy);
				const UniformBufferID visibleLightsBufferID = this->sceneManager.renderLightManager.getVisibleLightsBufferID();
				const int visibleLightCount = this->sceneManager.renderLightManager.getVisibleLightCount();
				const double screenSpaceAnimPercent = this->gameState.getChasmAnimPercent();

				const WorldDouble3 playerPosition = this->player.getEyePosition();
				const Degrees fovY = this->options.getGraphics_VerticalFOV();
				const double viewAspectRatio = this->window.getSceneViewAspectRatio();
				const double tallPixelRatio = RendererUtils::getTallPixelRatio(this->options.getGraphics_TallPixelCorrection());
				renderCamera.init(playerPosition, this->player.angleX, this->player.angleY, fovY, viewAspectRatio, tallPixelRatio);

				const ObjectTextureID paletteTextureID = this->sceneManager.gameWorldPaletteTextureRef.get();

				const bool isInterior = this->gameState.getActiveMapType() == MapType::Interior;
				const double dayPercent = this->gameState.getDayPercent();
				const bool isBefore6AM = dayPercent < 0.25;
				const bool isAfter6PM = dayPercent > 0.75;

				ObjectTextureID lightTableTextureID = this->sceneManager.normalLightTableDaytimeTextureRef.get();
				if (isFoggy)
				{
					lightTableTextureID = this->sceneManager.fogLightTableTextureRef.get();
				}
				else if (isInterior || isBefore6AM || isAfter6PM)
				{
					lightTableTextureID = this->sceneManager.normalLightTableNightTextureRef.get();
				}

				const DitheringMode ditheringMode = static_cast<DitheringMode>(this->options.getGraphics_DitheringMode());
				ObjectTextureID ditherTextureID = this->sceneManager.noneDitherTextureRef.get();
				if (ditheringMode == DitheringMode::Classic)
				{
					ditherTextureID = this->sceneManager.classicDitherTextureRef.get();
				}
				else if (ditheringMode == DitheringMode::Modern)
				{
					ditherTextureID = this->sceneManager.modernDitherTextureRef.get();
				}

				const ObjectTextureID skyBgTextureID = renderSkyManager.getBgTextureID();

				frameSettings.init(Colors::Black, ambientPercent, visibleLightsBufferID, visibleLightCount, screenSpaceAnimPercent, paletteTextureID,
					lightTableTextureID, ditherTextureID, skyBgTextureID, this->options.getGraphics_RenderThreadsMode(), ditheringMode);
			}

			this->uiManager.populateCommandList(uiCommandList);
			this->panel->populateCommandList(uiCommandList);

			for (const std::unique_ptr<Panel> &subPanel : this->subPanels)
			{
				subPanel->populateCommandList(uiCommandList);
			}

			const int profilerLevel = this->options.getMisc_ProfilerLevel();

			RenderElement2D debugInfoRenderElement;
			if (profilerLevel > Options::MIN_PROFILER_LEVEL)
			{
				this->updateDebugInfoText();

				const Int2 windowDims = this->window.getPixelDimensions();
				const Rect debugInfoTextBoxRect = this->debugInfoTextBox.getRect();
				const Rect debugInfoPresentRect = GuiUtils::makeWindowSpaceRect(debugInfoTextBoxRect.x, debugInfoTextBoxRect.y, debugInfoTextBoxRect.width, debugInfoTextBoxRect.height,
					UiPivotType::TopLeft, UiRenderSpace::Classic, windowDims.x, windowDims.y, this->window.getLetterboxRect());

				debugInfoRenderElement.id = this->debugInfoTextBox.getTextureID();
				debugInfoRenderElement.rect = debugInfoPresentRect;

				uiCommandList.addElements(Span<const RenderElement2D>(&debugInfoRenderElement, 1));

				if (profilerLevel >= 3)
				{
					this->debugQuadtreeState.populateCommandList(*this, uiCommandList);
				}
			}

			this->renderer.submitFrame(renderCommandList, uiCommandList, renderCamera, frameSettings);
		}
		catch (const std::exception &e)
		{
			DebugCrash("Render exception: " + std::string(e.what()));
		}

		// End-of-frame clean up.
		try
		{
			this->sceneManager.endFrame(this->physicsSystem, this->renderer);
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
