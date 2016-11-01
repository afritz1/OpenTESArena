#include <cassert>
#include <cmath>
#include <iostream>

#include "SDL.h"

#include "Game.h"

#include "GameState.h"
#include "../Utilities/Debug.h"

const int Game::MIN_FPS = 15;
const int Game::DEFAULT_FPS = 60;

Game::Game()
{
	Debug::mention("Game", "Platform is " + std::string(SDL_GetPlatform()) + ".");

	this->gameState = std::unique_ptr<GameState>(new GameState());
	this->targetFPS = Game::DEFAULT_FPS;
}

Game::~Game()
{

}

void Game::delay(int ms)
{
	assert(ms >= 0);
	SDL_Delay(static_cast<Uint32>(ms));
}

void Game::loop()
{
	// This loop doesn't check for SDL events itself. The current panel does that,
	// because most events like pressing "Esc" are context-sensitive.

    const int minimumMS = 1000 / Game::MIN_FPS;
    const int maximumMS = 1000 / this->targetFPS;
    int thisTime = SDL_GetTicks();
    int lastTime = thisTime;

	while (this->gameState->isRunning())
	{
        lastTime = thisTime;
        thisTime = SDL_GetTicks();

        int frameTime = thisTime - lastTime;
        if(frameTime < maximumMS)
        {
            this->delay(maximumMS - frameTime);
            thisTime = SDL_GetTicks();
            frameTime = thisTime - lastTime;
        }

        double dt = std::fmin(frameTime, minimumMS) / 1000.0;

		this->gameState->tick(dt);
		this->gameState->render();
	}
}
