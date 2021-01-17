#ifndef GAME_H
#define GAME_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "CharacterCreationState.h"
#include "GameData.h"
#include "Options.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Input/InputManager.h"
#include "../Interface/FPSCounter.h"
#include "../Interface/Panel.h"
#include "../Media/AudioManager.h"
#include "../Media/CinematicLibrary.h"
#include "../Media/DoorSoundLibrary.h"
#include "../Media/FontLibrary.h"
#include "../Media/MusicLibrary.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/Allocator.h"
#include "components/utilities/Profiler.h"

// This class holds the current game data, manages the primary game loop, and 
// updates the game state each frame.

// The game data holds all the active player and world data. It is null if a 
// game session is not currently running (in the main menu, character creation), 
// and is not null while a game session is running (in the game world, pause menu, 
// cinematic, journal, etc.).

// Game members should be available through a getter so panels can access them.

class Surface;

class Game
{
private:
	// A vector of sub-panels treated like a stack. The top of the stack is the back.
	// Sub-panels are more lightweight than panels and are intended to be like pop-ups.
	std::vector<std::unique_ptr<Panel>> subPanels;

	AudioManager audioManager;
	MusicLibrary musicLibrary;
	InputManager inputManager;
	FontLibrary fontLibrary;
	CinematicLibrary cinematicLibrary;
	CharacterClassLibrary charClassLibrary;
	DoorSoundLibrary doorSoundLibrary;
	EntityDefinitionLibrary entityDefLibrary;
	std::unique_ptr<GameData> gameData;
	std::unique_ptr<CharacterCreationState> charCreationState;
	Options options;
	std::unique_ptr<Panel> panel, nextPanel, nextSubPanel;
	Renderer renderer;
	TextureManager textureManager;
	BinaryAssetLibrary binaryAssetLibrary;
	TextAssetLibrary textAssetLibrary;
	Random random; // Convenience random for ease of use.
	ScratchAllocator scratchAllocator;
	Profiler profiler;
	FPSCounter fpsCounter;
	std::string basePath, optionsPath;
	bool requestedSubPanelPop;

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

	// Handles SDL events for the current frame.
	void handleEvents(bool &running);

	// Animates the game state by delta time.
	void tick(double dt);

	// Runs the current panel's render method for drawing to the screen.
	void render();
public:
	Game();
	Game(const Game&) = delete;
	Game(Game&&) = delete;

	Game &operator=(const Game&) = delete;
	Game &operator=(Game&&) = delete;

	// Gets the audio manager for changing the current music and sound.
	AudioManager &getAudioManager();

	// Gets the music library with all the music definitions.
	const MusicLibrary &getMusicLibrary() const;

	// Gets the input manager for obtaining input state. This should be read-only for
	// all classes except the Game class.
	InputManager &getInputManager();

	// Gets the font library for obtaining various fonts.
	FontLibrary &getFontLibrary();

	// Gets the cinematic library for obtaining various cinematic definitions.
	const CinematicLibrary &getCinematicLibrary() const;
	
	// Gets the character class library for obtaining various class definitions.
	const CharacterClassLibrary &getCharacterClassLibrary() const;

	// Gets the door sound library for obtaining various sound mappings.
	const DoorSoundLibrary &getDoorSoundLibrary() const;

	// Gets the entity definition library for obtaining various entity definitions.
	const EntityDefinitionLibrary &getEntityDefinitionLibrary() const;

	// Determines if a game session is currently running. This is true when a player
	// is loaded into memory.
	bool gameDataIsActive() const;

	// The game data holds the "session" data for the game. If no session is active, 
	// do not call this method. Verify beforehand by calling Game::gameDataIsActive().
	GameData &getGameData() const;

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

	// Gets various asset libraries for loading Arena-related files.
	const BinaryAssetLibrary &getBinaryAssetLibrary() const;
	const TextAssetLibrary &getTextAssetLibrary() const;

	// Gets the global RNG initialized at program start.
	Random &getRandom();

	// Gets the scratch buffer that is reset each frame.
	ScratchAllocator &getScratchAllocator();

	// Gets the profiler instance for measuring precise time spans.
	Profiler &getProfiler();

	// Gets the frames-per-second counter. This is updated in the game loop.
	const FPSCounter &getFPSCounter() const;

	// Sets the panel after the current SDL event has been processed (to avoid 
	// interfering with the current panel). This uses template parameters for
	// convenience (to avoid writing a unique_ptr at each callsite).
	template <class T, typename... Args>
	void setPanel(Args&&... args)
	{
		this->nextPanel = std::make_unique<T>(std::forward<Args>(args)...);
	}

	// Non-templated substitute for setPanel(), for when the panel takes considerable
	// effort at the callsite to construct (i.e., several parameters, intermediate
	// calculations, etc.).
	void setPanel(std::unique_ptr<Panel> nextPanel);

	// Adds a new sub-panel after the current SDL event has been processed (to avoid
	// adding multiple pop-ups from the same panel or sub-panel). This uses template 
	// parameters for convenience (to avoid writing a unique_ptr at each callsite).
	template <class T, typename... Args>
	void pushSubPanel(Args&&... args)
	{
		this->nextSubPanel = std::make_unique<T>(std::forward<Args>(args)...);
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

	// Sets the current game data object. A game session is active if the game data
	// is not null.
	void setGameData(std::unique_ptr<GameData> gameData);

	// Sets the current character creation state. Character creation is active if the state
	// is not null.
	void setCharacterCreationState(std::unique_ptr<CharacterCreationState> charCreationState);

	// Initial method for starting the game loop. This must only be called by main().
	void loop();
};

#endif
