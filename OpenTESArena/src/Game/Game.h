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
#include "../UI/UiContext.h"
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

	std::string nextContextName;

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
	UiContextInstanceID globalUiContextInstID; // For global UI elements/listeners like cursor and screenshots.
	UiElementInstanceID cursorImageElementInstID;
	UiTextureID defaultCursorTextureID; // Sword cursor used by most UI.

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
	bool running;

	void initOptions(const std::string &basePath, const std::string &optionsPath);

	// Resizes the SDL renderer and any other renderer-associated components.
	void resizeWindow(int width, int height);

	// Saves the given surface as a BMP file in the screenshots folder at the lowest
	// available index.
	void saveScreenshot(const Surface &surface);

	// Handles any change in the active UI context after an input event or game tick.
	void handleContextChanges();

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

	// Queues the UI context to switch to this frame. This must be one with begin/end/update callbacks registered at engine startup.
	void setNextContext(const char *contextName);

	// Sets the current character creation state. Character creation is active if the state
	// is not null.
	void setCharacterCreationState(std::unique_ptr<CharacterCreationState> charCreationState);

	// Sets the cursor texture + pivot, or defaults to sword cursor if no override.
	void setCursorOverride(const std::optional<UiCursorOverrideState> &state);

	// Initial method for starting the game loop. This must only be called by main().
	void loop();
};

#endif
