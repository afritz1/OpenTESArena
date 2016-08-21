#ifndef GAME_H
#define GAME_H

#include <memory>

// This class manages the primary game loop and updates the game state each frame.
// The actual game properties, current panel, and things relevant to the game are 
// in the "GameState" object and its "GameData" object.

class GameState;

class Game
{
private:
    static const int32_t MIN_FPS;
	static const int32_t DEFAULT_FPS;

	std::unique_ptr<GameState> gameState;
	int32_t targetFPS;

	// Waits the given number of milliseconds.
	void delay(int32_t ms);
public:
	Game();
	~Game();

	void loop();
};

#endif
