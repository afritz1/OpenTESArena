#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <memory>
#include <string>

#include "../Media/AudioManager.h"

// The game data object will hold all the player and world data. It will be null if 
// a game instance is not currently running (like when in the main menu), and will 
// not be null if a game instance is running (like when in the game world, pause menu, 
// cinematic, journal, etc.).

// Game state members should be public (through a method) so panels can access them.

class GameData;
class FontManager;
class Int2;
class Options;
class Panel;
class Renderer;
class TextureManager;
class TextAssets;

enum class MusicName;

class GameState
{
private:
	AudioManager audioManager;
	std::unique_ptr<FontManager> fontManager;
	std::unique_ptr<GameData> gameData;
	std::unique_ptr<MusicName> nextMusic;
	std::unique_ptr<Options> options;
	std::unique_ptr<Panel> panel, nextPanel;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<TextureManager> textureManager;
	std::unique_ptr<TextAssets> textAssets;
	bool running;

	void resizeWindow(int width, int height);
public:
	GameState();
	~GameState();

	bool isRunning() const;

	// This boolean determines if a session is currently running. This should be 
	// true when in the game world or in some form of pause menu or conversation.
	bool gameDataIsActive() const;

	FontManager &getFontManager() const;
	
	// This might be null, so it returns a pointer instead of a reference for safety.
	GameData *getGameData() const;

	AudioManager &getAudioManager();
	Options &getOptions() const;
	Renderer &getRenderer() const;
	TextureManager &getTextureManager() const;
	TextAssets &getTextAssets() const;

	// Set the next panel at the next tick.
	void setPanel(std::unique_ptr<Panel> nextPanel);

	// Set the next music at the next tick.
	void setMusic(MusicName name);

	void setGameData(std::unique_ptr<GameData> gameData);

	void tick(double dt);
	void render();
};

#endif
