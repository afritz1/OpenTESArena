#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <memory>
#include <string>

// I think the game state will hold all the player and world data pointers. They
// will be null if a game instance is not currently running (like when in the main
// menu), and will not be null if a game instance is running (like when in the game
// world, pause menu, journal, etc.).

class AudioManager;
class Panel;
class Renderer;
class TextureManager;

enum class MusicName;

class GameState
{
private:
	static const int DEFAULT_SCREEN_WIDTH;
	static const int DEFAULT_SCREEN_HEIGHT;
	static const bool DEFAULT_IS_FULLSCREEN;
	static const std::string DEFAULT_SCREEN_TITLE;

	std::unique_ptr<AudioManager> audioManager;
	std::unique_ptr<MusicName> nextMusic;
	std::unique_ptr<Panel> panel, nextPanel;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<TextureManager> textureManager;
	bool running;
public:
	GameState();
	~GameState();

	// Game state things should be public for panels to access them.

	bool isRunning() const;	
	AudioManager &getAudioManager() const;
	TextureManager &getTextureManager() const;
	std::unique_ptr<SDL_Rect> getLetterboxDimensions() const;

	void resizeWindow(int width, int height);

	// Set the next panel at the next tick.
	void setPanel(std::unique_ptr<Panel> nextPanel);

	// Set the next music at the next tick.
	void setMusic(MusicName name);

	void tick(double dt);
	void render();
};

#endif
