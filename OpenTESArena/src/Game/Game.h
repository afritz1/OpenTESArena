#ifndef GAME_H
#define GAME_H

#include <memory>

// This class manages the primary game loop and updates the game state each frame.
// The actual game properties, current panel, and things relevant to the game are 
// in the "GameState" object.

// Should options be in here or in the game state? I'm putting screen dimensions
// in the game state.

// Options should eventually be read from and written to an options.txt file.

class GameState;

class Game
{
private:
	static const int DEFAULT_FPS;

	std::unique_ptr<GameState> gameState;
	int targetFPS;

	// Waits the given number of milliseconds.
	void delay(int ms);
public:
	Game();
	~Game();

	void loop();
};

#endif
