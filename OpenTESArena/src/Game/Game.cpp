#include <cassert>
#include <cmath>
#include <iostream>

#include "SDL.h"

#include "Game.h"

#include "GameState.h"

const int32_t Game::MIN_FPS = 15;
const int32_t Game::DEFAULT_FPS = 60;

Game::Game()
{
	this->gameState = std::unique_ptr<GameState>(new GameState());
	this->targetFPS = Game::DEFAULT_FPS;
}

Game::~Game()
{

}

void Game::delay(int32_t ms)
{
	assert(ms >= 0);
	SDL_Delay(static_cast<Uint32>(ms));
}

void Game::loop()
{
	// This loop doesn't check for SDL events itself. The current panel does that,
	// because most events like pressing "Esc" are context-sensitive.

    const int32_t minimumMS = 1000 / Game::MIN_FPS;
    const int32_t maximumMS = 1000 / this->targetFPS;
    int32_t thisTime = SDL_GetTicks();
    int32_t lastTime = thisTime;

	while (this->gameState->isRunning())
	{
        lastTime = thisTime;
        thisTime = SDL_GetTicks();

        int32_t frameTime = thisTime - lastTime;
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
