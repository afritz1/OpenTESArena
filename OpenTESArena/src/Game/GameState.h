#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <memory>
#include <string>

// The game data object will hold all the player and world data. It will be null if 
// a game instance is not currently running (like when in the main menu), and will 
// not be null if a game instance is running (like when in the game world, pause menu, 
// cinematic, journal, etc.).

// Game state members should be public (through a method) so panels can access them.

class AudioManager;
class GameData;
class Int2;
class Panel;
class Renderer;
class TextureManager;

enum class MusicName;

struct SDL_Rect;

class GameState
{
private:
	static const int DEFAULT_SCREEN_WIDTH;
	static const int DEFAULT_SCREEN_HEIGHT;
	static const bool DEFAULT_IS_FULLSCREEN;
	static const std::string DEFAULT_SCREEN_TITLE;

	std::unique_ptr<AudioManager> audioManager;
	std::unique_ptr<GameData> gameData;
	std::unique_ptr<MusicName> nextMusic;
	// options object... to be coded at some point.
	std::unique_ptr<Panel> panel, nextPanel;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<TextureManager> textureManager;
	bool running;
public:
	GameState();
	~GameState();

	bool isRunning() const;

	// This boolean determines if a session is currently running. This should be 
	// true when in the game world or in some form of pause menu or conversation.
	bool gameDataIsActive() const;

	AudioManager &getAudioManager() const;

	// This might be null, so it returns a pointer instead of a reference for safety.
	GameData *getGameData() const;

	TextureManager &getTextureManager() const;

	Int2 getScreenDimensions() const;
	std::unique_ptr<SDL_Rect> getLetterboxDimensions() const;

	void resizeWindow(int width, int height);

	// Set the next panel at the next tick.
	void setPanel(std::unique_ptr<Panel> nextPanel);

	// Set the next music at the next tick.
	void setMusic(MusicName name);

	void setGameData(std::unique_ptr<GameData> gameData);

	void tick(double dt);
	void render();
};

#endif
