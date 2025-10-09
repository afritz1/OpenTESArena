#ifndef GAME_H
#define GAME_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "GameState.h"
#include "Options.h"
#include "../Assets/TextureManager.h"
#include "../Audio/AudioManager.h"
#include "../Input/InputManager.h"
#include "../Interface/GameWorldUiView.h"
#include "../Interface/Panel.h"
#include "../Player/CharacterCreationState.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Window.h"
#include "../UI/TextBox.h"
#include "../UI/UiManager.h"
#include "../World/ChunkManager.h"
#include "../World/SceneManager.h"

#include "components/utilities/FPSCounter.h"

class Surface;

// Holds the current game state, manages the primary game loop, and updates game state each frame.
// The game state holds all the active player and world data. It's empty if a game session is not
// currently running (in the main menu, character creation).
class Game
{
public:
	AudioManager audioManager;
	InputManager inputManager;
	std::unique_ptr<CharacterCreationState> charCreationState;
	Options options;
	Window window;
	Renderer renderer;
	TextureManager textureManager; // The texture manager object for loading images from file.
	JPH::PhysicsSystem physicsSystem; // The Jolt physics system for the scene.
	JPH::TempAllocatorImpl *physicsTempAllocator; // Available when game loop is active.

	// UI panels for the current interactivity and rendering sets. Needs to be positioned after the
	// renderer member in this class due to UI texture order of destruction (panels first, then renderer).
	std::unique_ptr<Panel> panel, nextPanel, nextSubPanel;

	// A vector of sub-panels treated like a stack. The top of the stack is the back.
	// Sub-panels are more lightweight than panels and are intended to be like pop-ups.
	std::vector<std::unique_ptr<Panel>> subPanels;

	// Screen regions for classic interface movement in the game world, scaled to fit the current window.
	Rect nativeCursorRegions[9];

	// Displayed with varying profiler levels.
	TextBox debugInfoTextBox;
	DebugVoxelVisibilityQuadtreeState debugQuadtreeState;

	Random random; // A modern random, use when accuracy to the original is not needed.
	ArenaRandom arenaRandom; // Replicates the original game's randomness.

	FPSCounter fpsCounter;

	SceneManager sceneManager;
	UiManager uiManager;

	// Active game session (needs to be positioned after Renderer member due to order of texture destruction).
	GameState gameState;

	Player player;

	// Whether the game loop should animate voxels, entities, and sky that can change over time.
	// Used when determining if the player is actively in the game world or in menus. This does 
	// not affect immediate operations like chunk management or scene transitions.
	bool shouldSimulateScene;

	// Whether to draw the 3D game world.
	bool shouldRenderScene;
private:
	// Listener IDs are optional in case of failed Game construction.
	std::optional<InputListenerID> applicationExitListenerID, windowResizedListenerID,
		renderTargetsResetListenerID, takeScreenshotListenerID, debugProfilerListenerID;

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

	void updateDebugInfoText();
public:
	Game();
	Game(const Game&) = delete;
	Game(Game&&) = delete;
	~Game();

	Game &operator=(const Game&) = delete;
	Game &operator=(Game&&) = delete;

	bool init();

	// Returns whether a new character is currently being created.
	bool characterCreationIsActive() const;

	// Gets the character creation state. Character creation must be active.
	CharacterCreationState &getCharacterCreationState() const;

	// Gets a UI rectangle used with classic game world interface for player movement.
	const Rect &getNativeCursorRegion(int index) const;

	// @todo: find a cleaner way to do this via listener callbacks or something.
	TextBox *getTriggerTextBox();
	TextBox *getActionTextBox();

	// Sets the panel after the current SDL event has been processed (to avoid 
	// interfering with the current panel). This uses template parameters for
	// convenience (to avoid writing a unique_ptr at each callsite).
	template<class T, typename... Args>
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
	template<class T, typename... Args>
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

	// Initial method for starting the game loop. This must only be called by main().
	void loop();
};

#endif
