#ifndef GAME_H
#define GAME_H

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "CharacterCreationState.h"
#include "GameState.h"
#include "Options.h"
#include "../Assets/TextureManager.h"
#include "../Audio/AudioManager.h"
#include "../Input/InputManager.h"
#include "../Interface/Panel.h"
#include "../Rendering/RenderChunkManager.h"
#include "../Rendering/Renderer.h"
#include "../UI/TextBox.h"
#include "../World/ChunkManager.h"
#include "../World/SceneManager.h"

#include "components/utilities/FPSCounter.h"
#include "components/utilities/Profiler.h"

// This class holds the current game state, manages the primary game loop, and 
// updates the game state each frame.

// The game state holds all the active player and world data. It is null if a 
// game session is not currently running (in the main menu, character creation), 
// and is not null while a game session is running (in the game world, pause menu, 
// cinematic, journal, etc.).

// Game members should be available through a getter so panels can access them.

class Surface;

class Game
{
public:
	using GameWorldRenderCallback = std::function<bool(Game&)>;
private:
	AudioManager audioManager;

	// Listener IDs are optional in case of failed Game construction.
	InputManager inputManager;
	std::optional<InputManager::ListenerID> applicationExitListenerID, windowResizedListenerID,
		takeScreenshotListenerID, debugProfilerListenerID;

	std::unique_ptr<CharacterCreationState> charCreationState;
	GameWorldRenderCallback gameWorldRenderCallback;
	Options options;
	Renderer renderer;
	TextureManager textureManager;

	// UI panels for the current interactivity and rendering sets. Needs to be positioned after the
	// renderer member in this class due to UI texture order of destruction (panels first, then renderer).
	std::unique_ptr<Panel> panel, nextPanel, nextSubPanel;

	// A vector of sub-panels treated like a stack. The top of the stack is the back.
	// Sub-panels are more lightweight than panels and are intended to be like pop-ups.
	std::vector<std::unique_ptr<Panel>> subPanels;

	// Screen regions for classic interface movement in the game world, scaled to fit the current window.
	std::array<Rect, 9> nativeCursorRegions;

	// Displayed with varying profiler levels.
	TextBox debugInfoTextBox;

	// Random number generators; the first is a modern random where accuracy to the original is not needed,
	// the second is meant to replicate the original game's.
	Random random;
	ArenaRandom arenaRandom;

	Profiler profiler;
	FPSCounter fpsCounter;

	SceneManager sceneManager;

	// Active game session (needs to be positioned after Renderer member due to order of texture destruction).
	GameState gameState;
	Player player;

	// Engine variables for what kinds of simulation should be attempted each frame.
	bool shouldSimulateScene;

	bool requestedSubPanelPop;
	bool running;

	// Gets the top-most sub-panel if one exists, or the main panel if no sub-panels exist.
	Panel *getActivePanel() const;

	void initOptions(const std::string &basePath, const std::string &optionsPath);

	// Resizes the SDL renderer and any other renderer-associated components.
	void resizeWindow(int width, int height);

	// Saves the given surface as a BMP file in the screenshots folder at the lowest
	// available index.
	void saveScreenshot(const Surface &surface);

	// Handles any changes in panels after an SDL event or game tick.
	void handlePanelChanges();

	void handleApplicationExit();
	void handleWindowResized(int width, int height);
	void updateNativeCursorRegions(int windowWidth, int windowHeight);

	// Optionally displays debug profiler info on-screen.
	void renderDebugInfo();
public:
	Game();
	Game(const Game&) = delete;
	Game(Game&&) = delete;
	~Game();

	Game &operator=(const Game&) = delete;
	Game &operator=(Game&&) = delete;

	bool init();

	// Gets the audio manager for changing the current music and sound.
	AudioManager &getAudioManager();

	// Gets the input manager for obtaining input state. This should be read-only for
	// all classes except the Game class.
	InputManager &getInputManager();

	// The game state holds the "session" for the game. If no session is active, do not call this method.
	// Verify beforehand by calling Game::gameStateIsActive().
	GameState &getGameState();

	Player &getPlayer();
	SceneManager &getSceneManager();

	// Whether the game loop should animate voxels, entities, and sky that can change over time.
	// Used when determining if the player is actively in the game world or in menus. This does 
	// not affect immediate operations like chunk management or scene transitions.
	bool isSimulatingScene() const;
	void setIsSimulatingScene(bool active);

	// Returns whether a new character is currently being created.
	bool characterCreationIsActive() const;

	// Gets the character creation state. Character creation must be active.
	CharacterCreationState &getCharacterCreationState() const;

	// Gets the options object for various settings (resolution, volume, sensitivity).
	Options &getOptions();

	// Gets the renderer object for rendering methods.
	Renderer &getRenderer();

	// Gets the texture manager object for loading images from file.
	TextureManager &getTextureManager();

	// Gets the global RNG initialized at program start.
	Random &getRandom();

	ArenaRandom &getArenaRandom();

	// Gets the profiler instance for measuring precise time spans.
	Profiler &getProfiler();

	// Gets the frames-per-second counter. This is updated in the game loop.
	const FPSCounter &getFPSCounter() const;

	// Gets a UI rectangle used with classic game world interface for player movement.
	const Rect &getNativeCursorRegion(int index) const;

	// @todo: find a cleaner way to do this via listener callbacks or something.
	TextBox *getTriggerTextBox();

	// Sets the panel after the current SDL event has been processed (to avoid 
	// interfering with the current panel). This uses template parameters for
	// convenience (to avoid writing a unique_ptr at each callsite).
	template <class T, typename... Args>
	void setPanel(Args&&... args)
	{
		std::unique_ptr<T> derivedPanel = std::make_unique<T>(*this);
		if (!derivedPanel->init(std::forward<Args>(args)...))
		{
			DebugCrash("Couldn't init new panel.");
		}

		this->nextPanel = std::move(derivedPanel);
	}

	// Adds a new sub-panel after the current SDL event has been processed (to avoid
	// adding multiple pop-ups from the same panel or sub-panel). This uses template 
	// parameters for convenience (to avoid writing a unique_ptr at each callsite).
	template <class T, typename... Args>
	void pushSubPanel(Args&&... args)
	{
		std::unique_ptr<T> derivedSubPanel = std::make_unique<T>(*this);
		if (!derivedSubPanel->init(std::forward<Args>(args)...))
		{
			DebugCrash("Couldn't init new sub-panel.");
		}

		this->nextSubPanel = std::move(derivedSubPanel);
	}

	// Non-templated substitute for pushSubPanel(), for when the sub-panel takes considerable
	// effort at the callsite to construct (i.e., several parameters, intermediate
	// calculations, etc.).
	void pushSubPanel(std::unique_ptr<Panel> nextSubPanel);

	// Pops the current sub-panel off the stack after the current SDL event has been
	// processed (to avoid popping a sub-panel while in use). This will normally be called 
	// by a sub-panel to destroy itself. If a new sub-panel is pushed during the same event,
	// then the old sub-panel is popped and replaced by the new sub-panel. Panels should 
	// never call this, because if they are active, then there are no sub-panels to pop.
	void popSubPanel();

	// Sets the current character creation state. Character creation is active if the state
	// is not null.
	void setCharacterCreationState(std::unique_ptr<CharacterCreationState> charCreationState);

	// Sets the function to call for rendering the 3D scene.
	void setGameWorldRenderCallback(const GameWorldRenderCallback &callback);

	// Initial method for starting the game loop. This must only be called by main().
	void loop();
};

#endif
